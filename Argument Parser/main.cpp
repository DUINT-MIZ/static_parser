#include <functional>
#include <iostream>
#include <array>
#include <type_traits>
#include <cstring>
#include "iterators.hpp"

#define MAX_ARG 10

/**
 * Parser, all of container was managed by user. (Trust me it's worth it)
 * for seasoned user :)))
 */

// Profile values was stored outside in stack

class Parser;

class storage_full : public std::runtime_error {
public:
    storage_full(const std::string& msg)
        : std::runtime_error(msg) {}
};

class prof_restriction : public std::runtime_error {
public:
    prof_restriction(const std::string& msg)
        : std::runtime_error(msg) {}
};

class token_error : public std::runtime_error {
public:
    token_error(const std::string& msg)
        : std::runtime_error(msg) {}
};


enum add_type : u_int8_t{
    Short = 1 << 0,
    Long = 1 << 1,
    LongShort = Long | Short
};

constexpr std::array<bool, 256> _make_table(){
    std::array<bool, 256> data {};

    for(int i = 0; i < 256; i++){ data[i] = false; }

    for(unsigned char c = '0'; c <= '9'; c++){
        data[c] = true;
    }

    for(unsigned char c = 'A'; c <= 'Z'; c++){
        data[c] = true;
    }

    for(unsigned char c = 'a'; c <= 'z'; c++){
        data[c] = true;
    }

    data['_'] = true;
    return data;
}

constexpr std::array<bool, 256> valid_char_table = _make_table();

bool valid_long_opt_name(const char* name)
{   
    if(name == nullptr) return false;
    if(std::strlen(name) < 3) return false;
    if((name[0] != '-') || (name[1] != '-')) return false;
    name += 2;
    while(name[0] != '\0') { 
        if(!valid_char_table[name[0]]) return false;
        name++;
    }
    return true;
}

bool valid_short_opt_name(const char* name)
{
    if(name == nullptr) return false;
    if(std::strlen(name) != 2) return false;
    return ((name[0] == '-') && ((name[1] == '_') ? false : valid_char_table[name[1]]));
}

bool valid_posarg_name(const char* name)
{
    if(name == nullptr) return false;
    while(name[0] != '\0') {
        if(!valid_char_table[name[0]]) return false;
        name++;
    }
    return true;
}

struct profile
{
    private :
    
    bool is_required : 1 = false;
    bool is_called : 1 = false;
    iterator_array<const char*> value;
    size_t narg;

    friend class Parser;
    
    public :
    
    // Determine wether it allow parser to store more than the profile::narg or not
    bool is_strict : 1 = false;
    
    /**
     * if option was called in arg, invoke profile::callback then exit(0).
     * this effect doesn't apply in parsing if this was assigned as posarg
     */
    bool is_immediate : 1 = false;

    /**
     * How many option calls are permitted,
     * if less than 0 then it's infinite
     */
    int permitted_call_count = 1;

    const char* lname = nullptr;
    unsigned char sname = '\0';

    
    std::function<void()> callback;

    iterator_viewer<const char*> value_viewer() const { return value.get_viewer(); }

    profile() = default;
    profile(profile&& oth) = default;
    profile& operator=(profile&&) = default;

    profile(const profile&) = delete;
    profile& operator=(const profile&) = delete;

    // Below function is just for chaining for cleaner code

    profile& set_strict(bool enable = true) noexcept {
        is_strict = enable;
        return *this;
    }

    profile& set_imme(bool enable = true) noexcept {
        is_immediate = enable;
        return *this;
    }

    profile& set_lname(const char* name) noexcept {
        lname = name;
        return *this;
    }

    profile& set_sname(unsigned char name) noexcept {
        sname = name;
        return *this;
    }

    profile& set_callback(const std::function<void()>& func) {
        callback = std::move(func);
        return *this;
    }

    profile& call_count(int call_count){
        permitted_call_count = call_count;
        return *this;
    }
};

class Parser {
    private :
    /**
     * long flag = '--'
     * short flag = '-'
     * posarg = no suffixes
     */
    std::unordered_map<std::string_view, profile*> lookup;

    iterator_array<profile> options;
    iterator_array<profile> posargs;
    
    /**
     * @brief Inserting data according to profile::narg
     * 
     * Number of argument stored at least satisfy profile::narg.
     * less will result in an error
     */
    void fetch_dat(
        iterator_viewer<const char*>& input_viewer,
        profile& prof,
        bool (*check_token)(const char* token) = [](const char* token){ return false; }
    ) const
    {
        std::cout << "Fetching Value started...\n";
        if(prof.permitted_call_count == 0) {
            std::string error_msg = "Permitted call count reaches 0, for : ";
            if(prof.lname) {
                error_msg += prof.lname;
            } else {
                error_msg.append(1, prof.sname);
            }
            throw prof_restriction(error_msg);
        }
        auto& value = prof.value;
        size_t narg_to_parse = prof.narg;
        const char* token;
        while(!input_viewer.end_reached())
        {
            if(value.full()) {
                std::cout << "Value full, breaking...\n";
                break;
            }

            if(narg_to_parse == 0) {
                if(prof.is_strict) {
                    std::cout << "(Strict Parsing). required narg finished, breaking...\n";
                    break;
                }
            }
            token = *input_viewer.get_val();
            std::cout << "Fetched : " << token << "\n";
            if(check_token(token)) {
                std::cout << "Token checking condition fulfilled, breaking... \n";
                break;
            }
            value.push_back(token);
            ++input_viewer;
            narg_to_parse--;
        }        

        if((signed)narg_to_parse > 0)
        {
            std::string error_msg = "Invalid number of argument passed for \"";
            if(prof.lname)
            {
                error_msg += prof.lname;
            } else {
                error_msg.append(1, prof.sname);
            }
            error_msg += "\", Needed : ";
            error_msg += std::to_string(prof.narg);
            throw prof_restriction(error_msg);
        }
        prof.is_called = true;
        --prof.permitted_call_count;
        std::cout << "Fetching value ended. \n";
    }

    void parse_opt(
        iterator_viewer<const char*>& argv_iter,
        iterator_array<const char*>& dump
    )
    { 
        std::cout << "Start of option parsing... \n";
        const char* token;
        
        profile* prof = nullptr;
        while(!argv_iter.end_reached())
        {
            token = *argv_iter.get_val();
            std::cout << "Fetching : " << token << "\n";
            if(token[0] == '-')
            {
                if(std::strcmp(token, "--") == 0) {
                    std::cout << "Special token, parsing option halted...\n";
                    break;
                }
                if(token[1] == '\0') {
                    std::cout << "Dumped...\n";
                    dump.push_back(token);
                }
                if((prof = get(token)) == nullptr) throw token_error(std::string("Unknown option of : ") + token);
                std::cout << "Identified, pointing at : " << (void*)prof << "\n";
                if(prof->is_immediate) {
                    std::cout << "is_immediate = True, Calling function\n";
                    prof->callback();
                    std::cout << "Exiting...\n";
                    std::exit(0);
                }
                ++argv_iter;
                fetch_dat(argv_iter, *prof, [](const char* token){ return (token[0] == '-'); });
            }
            else 
            {
                if(dump.full()) throw storage_full("Dump storage full parsing stopped");
                std::cout << "Dumped...\n";
                dump.push_back(token);
                ++argv_iter;
            }
        }
        std::cout << "End of option parsing..\n";
    }

    void parse_posarg(iterator_array<const char*>& dump)
    {   
        std::cout << "Start Parsing positional arguments..\n";
        auto dump_viewer = dump.get_viewer();
        auto posarg_viewer = posargs.get_viewer();
        
        while(!dump_viewer.end_reached())
        {
            if(posarg_viewer.end_reached()) break;
            std::cout << "Now fetching for : " << posargs[posarg_viewer.count_iterated()].lname << "\n";
            fetch_dat(
                dump_viewer,
                posargs[posarg_viewer.count_iterated()]
            );
            ++posarg_viewer;
        }
        if(!posarg_viewer.end_reached()) {
            std::string error_msg = "Insufficient narg passed, there is still ";
            error_msg += std::to_string(posarg_viewer.count_iterable());
            error_msg += " Posarg left to parse";
            throw prof_restriction(error_msg);
        }
        if(!dump_viewer.end_reached()) {
            throw token_error(std::string("Unexpected series of token starting from : ") + *dump_viewer.get_val());
        }
        std::cout << "End of positional argument parsing\n";
    }

    void finalize() {
        
        for(auto iter = options.begin(); iter < options.end_filled(); iter++){
            if(iter->is_required && !iter->is_called) {
                throw prof_restriction("A required option was not called");
            }
        }

        for(auto iter = options.begin(); iter < options.end_filled(); iter++){
            iter->callback();
        }
    }

    public :

    void test_map_out() const
    {
        std::cout << "Options : ";
        for(const profile* iter = options.begin(); iter < options.end_arr(); iter++)
        {
            if(!iter->lname){
                std::cout << "\"" << iter->sname << "\" ";
            } else 
            {
                std::cout << "\"" << iter->lname << "\" ";
            }
        }
        std::cout << std::endl;

        std::cout << "Posarg : ";
        for(const profile* iter = posargs.begin(); iter < posargs.end_arr(); iter++)
        {
            if(!iter->lname){
                std::cout << "\"" << iter->sname << "\" ";
            } else 
            {
                std::cout << "\"" << iter->lname << "\" ";
            }
        }
        std::cout << std::endl;
    }

    void set_immediate(profile& prof) const noexcept { prof.is_immediate = true; }

    template <size_t option_storage_size, size_t posarg_storage_size>
    Parser(
        std::array<profile, option_storage_size>& arr1,
        std::array<profile, posarg_storage_size>& arr2
    )
    {
        options.assign<option_storage_size>(arr1);
        posargs.assign<posarg_storage_size>(arr2);
    }

    bool exist(const std::string_view& name) const
    {
        return (lookup.count(name) != 0);
    }    

    profile* get(const std::string_view& name) const noexcept
    {
        auto it = lookup.find(name);
        if(it == lookup.end()) return nullptr;
        return it->second;
    }

    template <size_t storage_size>
    profile& add_opt(
        std::array<const char*, storage_size>& arr,
        add_type ins_type,
        bool is_required,
        size_t expected_narg,
        const char* lname,
        const char* sname
    )
    {
        
        if(options.full()) throw storage_full("Can't add option : Parser main storage full");
        
        if(!(ins_type & (Short | Long)))
        {
            throw std::invalid_argument("Can't add option : Invalid function argument");
        }
        
        profile p;
        p.value.assign<storage_size>(arr);
        p.narg = expected_narg;
        p.is_required = is_required;
        
        options.push_back(std::move(p));
        
        profile* ptr = &options[options.size() - 1];


        if(ins_type & Short)
        {   
            std::cout << "inserting short" << std::endl;
            if(!valid_short_opt_name(sname)) throw std::invalid_argument(std::string("Can't add option : Invalid short option name format, for : ") + sname);            
            auto [it, is_added] = lookup.emplace(sname, ptr);            
            if(!is_added) throw std::invalid_argument(std::string("Can't add option : Such a short option name exist for : ") + sname);
            ptr->sname = sname[1];
        }

        if(ins_type & Long) 
        {
            std::cout << "inserting long" << std::endl;
            if(!valid_long_opt_name(lname)) throw std::invalid_argument(std::string("Can't add option : Invalid long option name format, for : ") + lname);
            auto [it, is_added] = lookup.emplace(lname, ptr);
            if(!is_added) throw std::invalid_argument(std::string("Can't add option : Such a long option name exist for : ") + lname);
            ptr->lname = lname;
        }
        return *ptr;
    }

    template <size_t storage_size>
    profile& add_posarg(
        std::array<const char*, storage_size>& arr,
        size_t narg_expected,
        const char* name
    )
    {
        if(posargs.full()) throw storage_full("Can't add posarg : Parser main storage full");
        if((name == nullptr) || (narg_expected == 0)) {
            throw std::invalid_argument("Can't add posarg : Invalid function argument");
        }

        profile p;
        p.value.assign(arr);
        p.is_required = true;
        p.narg = narg_expected;
        posargs.push_back(std::move(p));
        profile* ptr = &posargs[posargs.size() - 1];

        if(!valid_posarg_name(name)) throw std::invalid_argument(std::string("Can't add posarg : Invalid posarg name format for : ") + name);
        auto [it, is_added] = lookup.emplace(name, ptr);
        if(!is_added) throw std::invalid_argument(std::string("Can't add posarg : Such a name already exist for : ") + name);
        ptr->lname = name;
        return *ptr;
    }
    template <typename... Args>
    void parse(Args&&... args){
        iterator_viewer<const char*> argv_viewer;
        argv_viewer.assign(std::forward<Args>(args)...);
        auto argc = argv_viewer.total_length();
        if(argc > MAX_ARG) {
            std::string error_msg = "Max argument are : ";
            error_msg += std::to_string(MAX_ARG);
            error_msg += " args, while you've entered ";
            error_msg += std::to_string(argc);
            throw std::overflow_error(error_msg);
        }
        const char* dump[MAX_ARG];
        iterator_array<const char*> dump_iter;
        dump_iter.assign(dump, argc);
        
        parse_opt(argv_viewer, dump_iter);
        parse_posarg(dump_iter);
        finalize();
    }
};



int main(int argc, char const *argv[])
{
    static std::array<profile, 3> options;
    static std::array<profile, 2> posarg;

    static std::array<const char*, 2> opt_val1;
    static std::array<const char*, 2> opt_val2;
    static std::array<const char*, 2> opt_val3;

    Parser parser(options, posarg);
    parser.add_opt<2>(opt_val1, Short, false, 2, nullptr, "-b")
        .set_callback([](){ std::cout << "-b called !" << std::endl; })
        .call_count(1)
        .set_strict(false);

    parser.add_opt<2>(opt_val2, Short, true, 2, nullptr, "-c")
        .set_callback([](){ std::cout << "-c called !" << std::endl; })
        .call_count(1)
        .set_strict();

    parser.add_opt<2>(opt_val2, Short, false, 2, nullptr, "-d")
        .set_callback([](){ std::cout << "-d called !" << std::endl; })
        .call_count(3)
        .set_strict();

    parser.parse(++argv, --argc);
}