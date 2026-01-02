#include <array>
#include <utility> // For std::move
#include <functional>
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <iostream>
#include <variant>
#include <cstring>
#include <charconv>
#include <type_traits>

#include "detail/values.hpp"
#include "detail/iterators.hpp"
#include "detail/parser_utils.hpp"

namespace parser {
namespace statis {

using namespace parser;

// Compile Time Evalution error class
class comtime_evaluation : public std::exception  {
    private :
    const char* msg;
    public :
    comtime_evaluation(const char* err_msg) : msg(err_msg) {}
    const char* what() noexcept { return msg; }
};

enum class FlowStatus {
    CONTINUE,
    ABORT
};

constexpr size_t string_length(const char* str) noexcept {
    size_t res = 0;
    while(*(str++) != '\0') ++res;
    return res;
}

struct static_profile {
    public :
    const char* lname                = nullptr;
    const char* sname                = nullptr;
    bool is_required                 = false;
    bool is_strict                   = false;
    bool is_immediate                = false;
    int permitted_call_count         = 0;
    std::size_t narg                 = 0;
    int exclude_point                = -1;
    values::TypeCode convert_code = values::TypeCode::UNDEFINED;

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

    constexpr static_profile& convert_to(values::TypeCode conv_code) noexcept {
        convert_code = conv_code;
        return *this;
    }

    constexpr static_profile& exclude_on(int point) {
        exclude_point = point;
        return *this;
    }
};

struct modifiable_profile {
    private :
    int times_called = 0;
    template <typename IdentifierCount, typename NumberOfPosarg> friend class Parser;
    
    public :
    #ifdef STATIC_PARSER_NO_HEAP
    using FunctionType = void (*)(static_profile, modifiable_profile&);
    #else
    
    using FunctionType = std::function<void(static_profile, modifiable_profile&)>;
    #endif
    FunctionType callback = [](static_profile, modifiable_profile&){};
    values::BoundValue value;
    
    modifiable_profile() = default;
    modifiable_profile(modifiable_profile&& oth) = default;
    modifiable_profile(const modifiable_profile& oth) = default;
    
    modifiable_profile& operator=(modifiable_profile&& oth) = default;

    modifiable_profile& operator=(const modifiable_profile& oth) = default;

    modifiable_profile& set_value(values::BoundValue& new_val) {
        value = std::move(new_val);
        return *this;
    }

    modifiable_profile& set_callback(const FunctionType& func) {
        callback = func;
        return *this;
    }

    modifiable_profile& self() noexcept { return *this; }

    constexpr int call_count() const noexcept { return times_called; }
};


template <std::size_t N> struct IDCount { static constexpr std::size_t value = N; };
template <std::size_t N> struct PosargCount { static constexpr std::size_t value = N; };
template <std::size_t N> struct DumpCapacity { static constexpr std::size_t value = N; };

template <size_t TotalProfiles>
// A Small class to hold an array of modifiable_profile that user cant access
class AligningData {
    static_assert((TotalProfiles > 0), "AligningData size must not be 0");
    private :
    std::array<modifiable_profile, TotalProfiles> arr;
    std::array<bool, TotalProfiles> initialized;
    std::array<const static_profile*, TotalProfiles> mutex_point{0};
    template <typename IdentifierCount, typename NumberOfPosarg> friend class Parser;
    
    void align(modifiable_profile&& mod_prof, std::size_t index) {
        if(index >= TotalProfiles) throw std::length_error("Align index is out of range (AligningData)");
        if(!initialized[index]) initialized[index] = true;
        arr[index] = std::move(mod_prof);
    }
    public :
};

template <typename IdentifierCount, typename NumberOfPosarg>
class Parser {
    static_assert(
        std::is_same_v<IdentifierCount, IDCount<IdentifierCount::value>> &&
        std::is_same_v<NumberOfPosarg, PosargCount<NumberOfPosarg::value>>,
        "Wrong template argument type. shouldve just use IDCount<N> and PosargCount<N>"
    );

    private :
    using map_type = frozen::unordered_map<frozen::string, const static_profile*, IdentifierCount::value>;

    map_type map;
    iterator_viewer<static_profile> profiles;
    std::array<const static_profile*, NumberOfPosarg::value> posargs_data{};
    iterator_array<const static_profile*> posargs;

    // Handles all of converting task.
    void convert(
        void* dest,
        values::TypeCode code,
        const char* token
    ) const {
        switch (code)
        {
        case values::TypeCode::INT :
            if(std::from_chars(token, token + std::strlen(token), *((int*)dest)).ec != std::errc())
                throw parse_error((std::string("Can't convert ") + token) + ", to an int");
            break;
            
        case values::TypeCode::DOUBLE :
            if(std::from_chars(token, token + std::strlen(token), *((double*)dest)).ec != std::errc())
                throw parse_error((std::string("Can't convert ") + token) + ", to a double");
            break;
        
        case values::TypeCode::STRING :
            *(reinterpret_cast<const char**>(dest)) = token;
            break;

        default:
            throw parse_error(std::string("Unknown type information, for flag/posarg associated with token = ") + token);
            break; 
        }
    }

    template <typename IteratorType>
    void fetch(
        modifiable_profile& mod_prof,
        const static_profile& static_prof,
        iterator_viewer<IteratorType>& input_iter,
        const char* token,
        bool (*check)(const char*) = [](const char* _){ return false; }
    ) const {
        switch (mod_prof.value.get_value_tag())
        {
        case values::ValTag::VAR_REF :
            convert(mod_prof.value.get_raw_ptr(), static_prof.convert_code, token);
            ++input_iter;
            break;
        
        case values::ValTag::ARRAY :
            {
                iterator_array<values::Blob>& value = mod_prof.value.get_array();
                std::size_t to_parse = static_prof.narg - value.count_inserted();
                values::TypeCode code = static_prof.convert_code;
                bool stopped_by_token_invalidation = false;
                bool stopped_by_full_value = false;
                arr_fetch:
                while((to_parse != 0) && !input_iter.end_reached()) {
                    token = *input_iter;

                    if(check(token)) {
                        stopped_by_token_invalidation = true;
                        break;
                    }

                    if(value.full()) {
                        stopped_by_full_value = true;
                        break;
                    }
                    value.push_back();
                    convert((void*)value.back().data(), code, token);

                    ++input_iter;
                    --to_parse;
                }

                if((signed)to_parse > 0) {
                    throw parse_error(
                        "Unsatisfied Narg need "
                        + std::to_string(static_prof.narg) + ", got "
                        + (
                            std::to_string(static_prof.narg - to_parse) 
                            + ", restricted by "
                        )
                        + (!static_prof.lname ? static_prof.sname : static_prof.lname));
                }

                if(stopped_by_token_invalidation ||
                    stopped_by_full_value ||
                    input_iter.end_reached()
                ) return;

                if(!static_prof.is_strict) {
                    to_parse = (unsigned)-1;
                    goto arr_fetch;
                } 
                
            }
            break;
        
        default:
            
            break;
        }
        ++mod_prof.times_called;
    }

    template <size_t N>
    FlowStatus handle_opt(
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
                ((0 <= token[1]) && (token[1] <= 9)) // if token is a possible negative number
                ) || 
                (token[0] != '-')
            ) {
                if(dump.full()) {
                    throw parse_error(
                        "Dump full, you can't pass more than "
                        + std::to_string(dump.total_capacity())
                        + " for posarg"
                    );
                }
                dump.push_back(token);
                ++argv_iter;
                continue;
            }

            if(std::strcmp(token, "--") == 0) return FlowStatus::CONTINUE; 

            equ_pos = find(token, '=');

            if(equ_pos != -1) {
                token[equ_pos] = '\0';
                it = map.find(frozen::string(token));
                token = &token[equ_pos + 1];
            } else {
                it = map.find(frozen::string(token));
                ++argv_iter;
            }

            if(it == map.end()) {
                throw parse_error(
                    (std::string("\"") + token)
                    + "\" Is an unknown flag"
                );
            }

            prof = it->second;
            mod_prof = &aligning_data.arr[prof - profiles.front()];
            
            if(prof->exclude_point > -1){
                if(
                    aligning_data.mutex_point[prof->exclude_point] != nullptr &&
                    aligning_data.mutex_point[prof->exclude_point] != prof
                ) {
                    prof = aligning_data.mutex_point[prof->exclude_point];
                    throw parse_error(
                        (
                            (std::string("Flag ") + *argv_iter)
                            + " Conflicted with "
                        ) + (prof->lname ? prof->lname : prof->sname)
                    );
                } else aligning_data.mutex_point[prof->exclude_point] = prof;
            }

            if(mod_prof->times_called >= prof->permitted_call_count) {
                throw parse_error(std::string("Permitted call_count reached, for ") + token);
            }

            fetch<char*>(
                aligning_data.arr[it->second - profiles.front()],
                *(it->second),
                argv_iter,
                token,
                [](const char* token){ return (token[0] == '-'); }
            );
            if(prof->is_immediate) {
                mod_prof->callback(*prof, *mod_prof);
                return FlowStatus::ABORT;
            }
        }
        return FlowStatus::CONTINUE;
    }
    template <size_t N>
    FlowStatus handle_posarg(
        AligningData<N>& aligning_data,
        iterator_array<const char*>& dump_arr
    ) const {
        auto posarg_iter = posargs.get_viewer();
        auto dump_iter = dump_arr.get_viewer();
        
        const char* tok;
        while(!dump_iter.end_reached() && !posarg_iter.end_reached()) {
            tok = *dump_iter;
            modifiable_profile& mod_prof = aligning_data.arr[(*posarg_iter - profiles.front())];
            fetch<const char*>(
                mod_prof,
                **(posarg_iter++),
                dump_iter,
                *dump_iter
            );
        }

        if(!posarg_iter.end_reached()) 
            throw parse_error("There is still another positional argument left to parse");
        
        if(!dump_iter.end_reached())
            throw parse_error("Unknown dump inputs, there is no positional argument left to parse...");
        
        
        return FlowStatus::CONTINUE;
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
            if(
                (profile.convert_code == values::TypeCode::UNDEFINED) &&
                (profile.narg != 0) 
            ) throw comtime_evaluation("Undefined convert_code... this convert code only permitted if narg was 0");

            if(profile.exclude_point >= (signed)profiles.total_size())
                throw comtime_evaluation("Exclude point was out of range");

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
        
        if(profiles.total_size() != N) {
            throw std::invalid_argument(
                "Aligning data length does not match with total profile"
            );
        }

        auto it = map.find(frozen::string(identifier_name));
        if(it == map.end()) throw std::invalid_argument(
            "Unknown identifier name, can't align data..."
        );

        switch (mod_prof.value.get_value_tag())
        {
        case values::ValTag::VAR_REF : 
            if(it->second->narg != 1)
                throw std::invalid_argument(
                        "Variable reference shouldn't be align with static_profile::narg != 1"
                    );

            if(mod_prof.value.get_type_code() != it->second->convert_code) {
                
                        
                throw std::invalid_argument(
                    "Variable reference type code mismatch"
                );
            }
            break;
        
        case values::ValTag::ARRAY :
            if(mod_prof.value.get_array().total_capacity() < it->second->narg)
                throw std::invalid_argument(
                    "total storage can't hold amount of value at least required by static_profile::narg"
                );
            break;
        
        case values::ValTag::NONE :
            if(it->second->narg == 0) {
                break;
            }
            throw std::invalid_argument(
                    "NONE Value type only permitted if corresponding aligning static_profile narg are 0"
                );
            break;
        
        default : 
            throw std::invalid_argument("Unknown value_type can't align data...");
            break;
        }
        std::ptrdiff_t diff = (it->second - profiles.front());
        // 
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
        
        if(handle_opt<N>(argv_iter, data, dump_arr) == FlowStatus::ABORT) return;
        
        if(handle_posarg<N>(data, dump_arr) == FlowStatus::ABORT) return;

        // Finalize
        std::size_t limit = profiles.total_size();
        const static_profile* prof = nullptr;
        modifiable_profile* mod_prof = nullptr;
        for(std::size_t i = 0; i < limit; i++) {
            if(profiles[i].is_required && !data.arr[i].times_called)
                throw parse_error("A required option/posarg was not called");
        }

        for(std::size_t i = 0; i < limit; i++) {
            modifiable_profile& prof = data.arr[i];
            if(prof.times_called)
                prof.callback(profiles[i], prof);
        }
    }

};


template <std::size_t N>
struct tag {};
// 
// template <std::size_t N>
// constexpr std::size_t ret_template_arg(const tag<N>& _) { return N; }

template <std::size_t N>
constexpr std::size_t count_id(const std::array<static_profile, N>& arr) {
    std::size_t count{0};
    for(const auto& prof : arr) {
        if(prof.lname) ++count;
        if(prof.sname) ++count;
    }
    return count;
}

template <std::size_t N>
constexpr std::size_t count_posarg(const std::array<static_profile, N>& arr) {
    std::size_t count{0};
    for(const auto& prof : arr) {
        if(prof.lname) {
            if((prof.lname[0] != '-') && !prof.sname) ++count;
        }
    }
    return count;
}

using temp_pair = std::pair<const char*, const static_profile*>;

template <std::size_t id_count, std::size_t N, std::size_t... Is>
constexpr auto make_map(const std::array<static_profile, N>& profiles, tag<id_count>, std::index_sequence<Is...>) {
    std::array<temp_pair, id_count> temp{};

    std::size_t curr_temp_idx = 0;
    for(const auto& prof : profiles) {
        if(prof.lname) {
            temp[curr_temp_idx] = {prof.lname, &prof};
            ++curr_temp_idx;
        }

        if(prof.sname) {
            temp[curr_temp_idx] = {prof.sname, &prof};
            ++curr_temp_idx;
        }
    }

    std::array<std::pair<frozen::string, const static_profile*>, id_count> profile_pairs = {{
        {frozen::string(temp[Is].first), temp[Is].second}...
    }};
    return frozen::make_unordered_map<frozen::string, const static_profile*>(profile_pairs);
}

template <typename F, std::size_t N>
consteval auto make_parser(F get_arr, const std::array<static_profile, N>& _) {
    static_assert(
        std::is_same_v<decltype(get_arr()), decltype(_)>,
        "get_arr must be at least []() -> const auto& { return <constexpr std::array>; }"
    );
    constexpr auto& arr = get_arr();
    constexpr auto id_count = count_id(arr);
    auto map = make_map(arr, tag<id_count>{}, std::make_index_sequence<id_count>{});
    return Parser<IDCount<id_count>, PosargCount<count_posarg(arr)>>(map, arr);
}

}
}