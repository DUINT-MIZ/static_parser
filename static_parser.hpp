#include <array>
#include <utility> // For std::move
#include <functional>
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <iostream>
#include <charconv>

#include "detail/iterators.hpp"
#include "detail/polytypes.hpp"
#include "detail/parser_utils.hpp"

namespace parser {
namespace statis {

using namespace parser;

// Compile Time Evalution error class
class comtime_evaluation : public std::exception  {
    private :
    std::string msg;
    public :
    comtime_evaluation(const std::string& err_msg) : msg(err_msg) {}
    const char* what() noexcept { return msg.data(); }
};

enum class val_type {
    ARRAY,
    VAR_REF,
    NONE
};

struct static_profile {
    private :
    std::size_t index = 0;
    template <typename IdentifierCount, typename NumberOfPosarg> friend class Parser;

    public :
    const char* lname                = nullptr;
    const char* sname                = nullptr;
    bool is_required                 = false;
    bool is_strict                   = false;
    bool is_immediate                = false;
    uint64_t conflict_mask           = 0;
    int permitted_call_count         = 0;
    std::size_t narg                 = 0;
    polytype::type_code convert_code = polytype::type_code::UNDEFINED;

    constexpr static_profile& identifier(const char* new_lname, const char* new_sname) noexcept {
        lname = new_lname;
        sname = new_sname;
        return *this;
    }

    constexpr static_profile& behavior(uint flag) noexcept {
        is_required = (flag & behave::IS_REQUIRED);
        is_strict = (flag & behave::IS_STRICT);
        is_immediate = (flag & behave::IS_IMMEDIATE);
        return *this;
    }

    constexpr static_profile& call_count(std::size_t new_call_count) noexcept {
        permitted_call_count = new_call_count;
        return *this;
    }

    constexpr static_profile& set_narg(std::size_t new_narg) noexcept {
        narg = new_narg;
        return *this;
    }

    constexpr static_profile& conflict(std::size_t new_conflict) noexcept {
        conflict_mask = new_conflict;
        return *this;
    }

    constexpr static_profile& convert_to(polytype::type_code conv_code) noexcept {
        convert_code = conv_code;
        return *this;
    }
};

struct modifiable_profile {
    private :
    int times_called = 0;
    val_type value_type = val_type::NONE;
    union {
        polytype::NumberedPtr ptr_val;
        iterator_array<polytype::Values> arr_val;
    } value;

    template <typename IdentifierCount, typename NumberOfPosarg> friend class Parser;

    public :
    
    std::function<void(static_profile, modifiable_profile&)> callback;

    modifiable_profile() : value{} {}
    template <typename T>
    typename std::enable_if<polytype::restype_supported<T>::value, modifiable_profile&>::type
    bind(T& val) {
        value.ptr_val = val;
        value_type = val_type::VAR_REF;
        return *this;
    }

    template <typename... Args>
    modifiable_profile& bind(Args&&... iterator_array_constructor_arg) {
        value.arr_val = iterator_viewer(
            std::forward<Args>(iterator_array_constructor_arg)...
        );
        value_type = val_type::ARRAY;
        return *this;
    }

    modifiable_profile& set_callback(const std::function<void(static_profile, modifiable_profile&)>& func) {
        callback = func;
        return *this;
    }

    modifiable_profile& operator=(modifiable_profile&& other) {
        if (this == &other) return *this;

        this->times_called = other.times_called;
        this->value_type = other.value_type;
        this->callback = std::move(other.callback);

        if (other.value_type == val_type::ARRAY) {
            this->value.arr_val = std::move(other.value.arr_val);
        } 
        else if (other.value_type == val_type::VAR_REF) {
            this->value.ptr_val = other.value.ptr_val;
        }
        
        return *this;
    }

    polytype::NumberedPtr& get_ptr() {
        if(value_type == val_type::VAR_REF) {
            return value.ptr_val;
        } else {
            throw std::bad_cast();
        }
    }
    
    iterator_array<polytype::Values>& get_arr() {
        if(value_type == val_type::ARRAY) {
            return value.arr_val;
        } else {
            throw std::bad_cast();
        }
    }
};


template <std::size_t N> struct IDCount { static constexpr std::size_t value = N; };
template <std::size_t N> struct PosargCount { static constexpr std::size_t value = N; };
template <std::size_t N> struct DumpCapacity { static constexpr std::size_t value = N; };

template <size_t TotalProfiles>
// A Small class to hold an array of modifiable_profile that user can't access
class AligningData {
    static_assert((TotalProfiles > 0), "AligningData size must not be 0");
    private :
    std::array<modifiable_profile, TotalProfiles> arr;
    std::array<bool, TotalProfiles> initialized;
    template <typename IdentifierCount, typename NumberOfPosarg> friend class Parser;
    
    void align(modifiable_profile&& mod_prof, std::size_t index) {
        if(index >= TotalProfiles) throw std::length_error("Align index is out of range (AligningData)");
        if(!initialized[index]) initialized[index] = true;
        arr[index] = std::move(mod_prof);
    }
    public :
};

class ServedData {
    private :
    modifiable_profile* start = nullptr;
    modifiable_profile* end = nullptr;
    std::function<std::size_t(const char*)> find_position;

    public :
    ServedData() = delete;

    template <size_t N>
    ServedData(std::pair<AligningData<N>&, std::function<std::size_t(const char)>&&> d)
        : find_position(std::move(d.second))
    {
        start = d.first.arr.data();
        end = &d.first.arr.back() + 1;
    }

    modifiable_profile& get(const char* name) {
        auto idx = find_position(name);
        if((signed)idx == -1) throw std::invalid_argument("Get failed, unknown name");
        return start[idx];
    }

};

template <typename IdentifierCount, typename NumberOfPosarg>
class Parser {
    static_assert(
        std::is_same_v<IdentifierCount, IDCount<IdentifierCount::value>> &&
        std::is_same_v<NumberOfPosarg, PosargCount<NumberOfPosarg::value>>,
        "Wrong template argument type. should've just use IDCount<N> and PosargCount<N>"
    );

    private :
    using map_type = frozen::unordered_map<frozen::string, const static_profile*, IdentifierCount::value>;

    const map_type& map;
    iterator_viewer<static_profile> profiles;
    std::array<const static_profile*, NumberOfPosarg::value> posargs_data;
    iterator_array<const static_profile*> posargs;

    void convert(
        void* dest,
        polytype::type_code code,
        const char* str
    ) const {
        switch (code)
        {
        case polytype::type_code::INT :
            *(reinterpret_cast<int*>(dest)) = std::stoi(str);
            break;
            
        case polytype::type_code::DOUBLE :
            *(reinterpret_cast<double*>(dest)) = std::stod(str);
            break;
        
        case polytype::type_code::STRING :
            *(reinterpret_cast<const char**>(dest)) = str;
            break;

        default:
            throw std::runtime_error("Can't convert token because of unknown type information...");
            break;
        }
    }
    // Handles all of converting task.
    template <typename IteratorType>
    void fetch(
        modifiable_profile& mod_prof,
        const static_profile& static_prof,
        iterator_viewer<IteratorType>& input_iter,
        const char* token,
        bool (*check)(const char*) = [](const char* _){ return false; }
    ) const {
        
        switch (mod_prof.value_type)
        {
        case val_type::VAR_REF :
            std::cout << "Variable reference insert" << std::endl;
            convert(
                mod_prof.value.ptr_val.ptr,
                static_prof.convert_code,
                token
            );
            ++input_iter;
            break;
        
        case val_type::ARRAY :
            {
                iterator_array<polytype::Values>& value = mod_prof.value.arr_val;
                std::size_t to_parse = static_prof.narg - value.count_inserted();
                polytype::type_code code = static_prof.convert_code;
                bool stopped_by_token_invalidation = false;
                arr_fetch:
                while((to_parse != 0) && !input_iter.end_reached()) {
                    token = *input_iter;

                    if(check(token)) {
                        stopped_by_token_invalidation = true;
                        break;
                    }

                    convert(
                        &value.back().buf,
                        code,
                        token
                    );

                    ++input_iter;
                    --to_parse;
                }

                if((signed)to_parse > 0) {
                    throw std::runtime_error("Invalid number of argument received");
                }
                if(stopped_by_token_invalidation) return;
                if(!static_prof.is_strict) {
                    // Not stricted, then take anything until token criteria are met
                    to_parse = (unsigned)-1;
                    goto arr_fetch;
                } 
                
            }
            break;
        
        default:
            break;
        }
    }

    template <size_t N>
    void handle_opt(
        iterator_viewer<char*>& argv_iter,
        AligningData<N>& aligning_data,
        iterator_array<const char*>& dump
    ) const {
        char* token;
        int equ_pos;
        auto it = map.end();
        uint64_t conflicts = 0;
        const static_profile* prof = nullptr;
        modifiable_profile* mod_prof = nullptr;
        while(!argv_iter.end_reached()) {
            token = *argv_iter;
            
            if(
                (
                (token[0] == '-') &&
                (('0' <= token[1]) && (token[1] <= '9')) // if token is a possible negative number
                ) || 
                (token[0] != '-')
            ) {
                dump.push_back(token);
                ++argv_iter;
                continue;
            }

            equ_pos = find(token, '=');

            if(equ_pos != -1) {
                token[equ_pos] = '\0';
                it = map.find(frozen::string(token));
                token = &token[equ_pos + 1];
            } else {
                it = map.find(frozen::string(token));
                token = *(++argv_iter);
            }

            if(it == map.end()) {
                throw std::runtime_error("Unknown flag is passed");
            }

            prof = it->second;
            mod_prof = &aligning_data.arr[prof - profiles.front()];
            if(conflicts & prof->conflict_mask) {
                throw std::runtime_error("Flags Conflicted");
            } else conflicts |= prof->conflict_mask;

            if(mod_prof->times_called++ >= prof->permitted_call_count) {
                throw std::runtime_error("Permitted call_count reached");
            }

            fetch<char*>(
                aligning_data.arr[it->second - profiles.front()],
                *(it->second),
                argv_iter,
                token,
                [](const char* token){ return (token[0] == '-'); }
            );
            std::cout << "argv_iter pos (after fetch) : " << argv_iter.count_iterated() << std::endl;
        }
    }
    template <size_t N>
    void handle_posarg(
        AligningData<N>& aligning_data,
        iterator_array<const char*>& dump_arr
    ) const {
        auto posarg_iter = posargs.get_viewer();
        auto dump_iter = dump_arr.get_viewer();
        const char* tok;
        while(!dump_iter.end_reached() && !posarg_iter.end_reached()) {
            tok = *dump_iter;
            fetch<const char*>(
                aligning_data.arr[(*posarg_iter - posargs.front())],
                **(posarg_iter++),
                dump_iter,
                *dump_iter
            );
        }

        if(!posarg_iter.end_reached()) 
            throw std::runtime_error("There is still another positional argument left to parse");
        
        if(!dump_iter.end_reached())
            throw std::runtime_error("Unknown dump inputs, there is no positional argument left to parse...");
    }

    constexpr void verify_name(const static_profile* target, const char* name) {
        auto it = map.end();
        if((it = map.find(frozen::string(name))) != map.end()) {
            if(it->second != target) {
                throw comtime_evaluation("Identifier does not refer to it's original static_profile");
            }
        } else throw comtime_evaluation("Identifier not registered in map");
    }

    public : 

    template <typename... Args>
    constexpr Parser(
        const map_type& new_map,
        Args&&... iterator_array_constructor_arg
    ) 
        : posargs(posargs_data),
          map(new_map),
          profiles(std::forward<Args>(iterator_array_constructor_arg)...) 
    {   
        std::size_t valid_mappings = 0;
        std::size_t valid_mappings_2 = 0;
        for(auto& profile : profiles) {
            if(profile.convert_code == polytype::type_code::UNDEFINED) throw comtime_evaluation("Undefined convert_code");

            if(profile.lname) {
                if(profile.lname[0] == '-') {
                    if(!valid_long_opt_name(profile.lname, '-')) throw comtime_evaluation("Invalid long option name"); 
                } else {
                    if(profile.sname)
                        throw comtime_evaluation("Positional argument should not have a short name identifier");
                    if(!valid_posarg_name(profile.lname)) throw comtime_evaluation("Invalid posarg name");
                    posargs.push_back(&profile);
                }
                verify_name(&profile, profile.lname);
                ++valid_mappings;
            }

            if(profile.sname) {
                if(!valid_short_opt_name(profile.sname, '-')) throw comtime_evaluation("Invalid short option name");
                verify_name(&profile, profile.sname);
                ++valid_mappings;
            }
            if(valid_mappings == valid_mappings_2) throw comtime_evaluation("Empty identifier is not allowed");
            valid_mappings_2 = valid_mappings;
        }
        if(valid_mappings != IdentifierCount::value) {
            throw comtime_evaluation("Invalid number of map valid identifier to map key (forgot to assign key to profile identifier ?)");
        }
    }

    template <size_t N>
    modifiable_profile& align(
        AligningData<N>& aligning_data,
        const char* identifier_name,
        modifiable_profile& mod_prof
    ) const {
        {
        bool* iter = aligning_data.initialized.data();
        bool* end = &aligning_data.initialized.back() + 1;
        while(*(iter++) && (iter != end)){}
        if(iter != end)
            throw std::invalid_argument(
                "Aligning data does not fully initialized"
            );
        }

        if(profiles.total_size() != N) {
            throw std::invalid_argument(
                "Aligning data length does not match with total profile"
            );
        }

        auto it = map.find(frozen::string(identifier_name));
        if(it == map.end()) throw std::invalid_argument(
            "Unknown identifier name, can't align data..."
        );

        switch (mod_prof.value_type)
        {
        case val_type::VAR_REF :
            if(mod_prof.value.ptr_val.code != it->second->convert_code){
                throw std::invalid_argument(
                    "Variable reference type information does not match with static_profile::convert_type"
                );
            }
            if(it->second->narg != 1)
                throw std::invalid_argument(
                        "Variable reference shouldn't be align with static_profile::narg != 1"
                    );
            break;
        
        case val_type::ARRAY :
            if(mod_prof.value.arr_val.total_capacity() < it->second->narg)
                throw std::invalid_argument(
                    "total storage can't hold amount of value at least required by static_profile::narg"
                );
            break;
        
        default:
            throw std::invalid_argument("Unknown value_type can't align data...");
            break;
        }
        std::ptrdiff_t diff = (it->second - profiles.front());
        // std::cout << "ptrdiff = " << diff << std::endl;
        aligning_data.align(std::move(mod_prof), diff);
        return aligning_data.arr[diff];
    }

    template <size_t N, typename dump_size>
    void parse(AligningData<N>& data, char** argv, int argc) const {
        static_assert(
            std::is_same<dump_size, DumpCapacity<dump_size::value>>::value,
            "template argument dump_size must be inserted with DumpCapacity<N>"
        );
        
        if(profiles.total_size() != N)
            throw std::runtime_error("Invalid alinging data with total profiles, parsing fail");
        
        std::array<const char*, dump_size::value> dump_dat = {};
        iterator_array<const char*> dump_arr(dump_dat);
        iterator_viewer<char*> argv_iter(argv, argc);

        handle_opt<N>(argv_iter, data, dump_arr);
        handle_posarg<N>(data, dump_arr);

        // Finalize
        std::size_t limit = profiles.total_size();
        const static_profile* prof = nullptr;
        modifiable_profile* mod_prof = nullptr;
        for(std::size_t i = 0; i < limit; i++) {
            auto something = profiles[i];
        }


    }
};

}

}