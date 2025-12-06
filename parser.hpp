#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <list>
#include <chrono>
#include <bitset>
#include "iterators.hpp"
/*
iterator_viewer are more practical than std::span in this context
it's basically your normal STL'ish iterator
the difference is it's not a subclass
and you don't gotta bring another variable to mark where you are !
*/


void vec_map(std::vector<const char*>& vec) {
    std::cout << "{ ";
    for(auto& e : vec) {
        std::cout << "\'" << e << "\', ";
    }
    std::cout << "}\n";
}

namespace parser {

constexpr std::array<bool, 256> identifier_make_table(){
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

constexpr auto identifier_char_table = identifier_make_table();

namespace behave {
    constexpr uint IS_STRICT = 1 << 2;
    constexpr uint IS_REQUIRED = 1 << 1;
    constexpr uint IS_IMMEDIATE = 1 << 0;
};

struct profile {
    private : 
    std::string lname = "";
    std::string sname = "";
    int permitted_call_count = 1;
    bool is_required = false;
    bool is_strict = false;
    bool is_immediate = false;
    std::size_t narg = 0;
    std::uint64_t conflict_mask = 0;
    std::vector<const char*> value;
    int times_called = 0;
    
    friend class Parser;
    friend class parse_error;

    public : 
    
    std::function<void(profile& _)> callback = [](profile& _){};
    
    profile() = default;
    profile(profile&& oth) = default;

    profile& identifier(const char* new_lname, const char* new_sname) noexcept {
        lname = new_lname;
        sname = new_sname;
        return *this;
    }

    profile& behavior(uint flag) noexcept {
        is_required = (flag & behave::IS_REQUIRED);
        is_strict = (flag & behave::IS_STRICT);
        is_immediate = (flag & behave::IS_IMMEDIATE);
        return *this;
    }

    profile& set_narg(std::size_t new_narg) {
        narg = new_narg;
        value.reserve(new_narg);
        return *this;

    }
    profile& conflict(std::uint64_t mask) noexcept {
        conflict_mask = mask;
        return *this;
    }

    profile& call_count(std::size_t new_call_count) noexcept {
        permitted_call_count = new_call_count;
        return *this;
    }

    int call() {
        return times_called;
    }
};

namespace code {
    constexpr uint FINALIZE      = 0b10 << 0;
    constexpr uint OPT_PARSE     = 0b0 << 0;
    constexpr uint POSARG_PARSE  = 0b1 << 0;
    
    constexpr uint UNKNOWN_TOKEN = 0b1  << 2; // Unknown Token
    constexpr uint INS_NARG      = 0b10 << 2; // Insufficient NArg
    constexpr uint UNCALLED_REQ  = 0b11 << 2; // Uncalled Required
    constexpr uint CALL_LIMIT    = 0b1  << 4;
    constexpr uint CONFLICT      = 0b10 << 4;

    constexpr uint TYPE_MASK = (unsigned)(-1) << 2;
    constexpr uint FROM_MASK = 0b11;
}

struct error_bio {
    uint code;
    std::size_t position;
    profile restricter;

    error_bio(uint new_code, std::size_t pos)
        : code(new_code), position(pos) {}

    error_bio(uint new_code, std::size_t pos, profile& prof)
        : code(new_code), position(pos), restricter(std::move(prof)) {}

    error_bio(error_bio&& oth) = default;
    error_bio() = default;
};

class parse_error : public std::runtime_error {
    public : 
    error_bio bio;

    static std::string make_message(error_bio& bio) {
        std::bitset<32> code_bits = bio.code;
        std::cout << "DEBUG : " << code_bits.to_string() << std::endl;

        std::string msg = "Parsing stopped... ";
        
        switch (bio.code & code::FROM_MASK)
        {
        case code::OPT_PARSE :
            msg += "(from parsing option)\n\tat position \033[1m\033[93m";
            msg += std::to_string(bio.position) + "\033[0m of\033[92m\033[1m argument passed\033[0m";
            break;
        
        case code::POSARG_PARSE :
            msg += "(from parsing posarg)\n\tat position \033[1m\033[93m";
            msg += std::to_string(bio.position) + "\033[0m of\033[92m\033[1m positional argument input\033[0m\n";
            break;
        
        case code::FINALIZE :
            msg += "(from finalize)\n\tat position none\n";
            break;
        
        default:
            break;
        }

        msg += "\nRestricted by \033[1m\033[36m";
        if(bio.restricter.lname.empty() && bio.restricter.sname.empty()) {
            msg += "none";
        } else if (!bio.restricter.lname.empty()) {
            msg += bio.restricter.lname;
        } else {
            msg += bio.restricter.sname;
        }
        msg += "\033[0m\n\t=> ";

        switch(bio.code & code::TYPE_MASK) {

        case code::CALL_LIMIT :
            msg += "\033[31m\033[1mExceeding Call Limit\033[0m\n\t\t-> Permitted \033[32m\033[1m"
                +  std::to_string(bio.restricter.permitted_call_count) + "\033[0m, You called \033[33m\033[1m"
                +  std::to_string(bio.restricter.times_called) + "\033[0m\n";
            break;
        
        case code::CONFLICT :
            {
                std::bitset<64> mask_on_bin(bio.restricter.conflict_mask);
                msg += "\033[31m\033[1mConflict\033[0m With mask of \033[4m" + mask_on_bin.to_string() + "\033[0m\n";
            }
            break;
            
        case code::INS_NARG : 
            msg += "\033[31m\033[1mInsufficient NArg passed\033[0m\n\t\t-> Required \033[32\033[1m"
            +  std::to_string(bio.restricter.narg) + "\033[0m, got \033[33m\033[1m" + std::to_string(bio.restricter.value.size())
            + "\033[0m\n";
            break;

        case code::UNCALLED_REQ :
            msg += "Is a \033[31m\033[1mRequired\033[0m, but never call it\n";
            break;

        case code::UNKNOWN_TOKEN :
            msg += "Unknown token is passed\n";
        }

        return msg;
    }

    parse_error(error_bio&& new_bio) : bio(std::move(new_bio)), std::runtime_error(make_message(new_bio)) {}
};

inline int find(const char* str, char c) {
    int res = -1;
    char c1;
    for(int i = 0; (c1 = str[i]) != '\0'; i++) { if(c1 == c) res = i; }
    return res;
}

inline bool is_num(char c) { return (('0' <= c) && (c <= '9')); }

class Parser {
    private : 

    using opt_map = std::unordered_map<std::string_view, profile*>;
    using sbc_map = std::unordered_map<std::string, std::function<void(const char**, int)>>;
    opt_map map;
    sbc_map subparser;
    std::list<profile> profiles;
    std::vector<profile*> posargs;
    char flag_pref = '-';

    inline void handle_opt(
        iterator_viewer<const char*>& input_viewer,
        profile& prof,
        std::uint64_t& conflicts
    ) {
        const char* token = *input_viewer;
        if(prof.times_called++ == prof.permitted_call_count) {
            throw parse_error(
                error_bio(
                    code::OPT_PARSE | code::CALL_LIMIT,
                    input_viewer.count_iterated(),
                    prof
                )
            );
        }
        if(conflicts & prof.conflict_mask) {
            throw parse_error(
                error_bio(
                    code::OPT_PARSE | code::CONFLICT,
                    input_viewer.count_iterated(),
                    prof
                )
            );
        } else conflicts |= prof.conflict_mask;

        if((prof.value.size() >= prof.narg) && prof.is_strict) {
            ++input_viewer;
            return;
        }
        int eq_pos;
        if((eq_pos = find(token, '=')) != -1) {
            if(*(token = &token[eq_pos + 1])) prof.value.push_back(token);
        } else {
            std::size_t to_parse = prof.narg - prof.value.size();
            for(; to_parse > 0; --to_parse) {
                if((++input_viewer).end_reached()) break;
                if((token = *input_viewer)[0] == '-') break;
                prof.value.push_back(token);
            }
        }

        if(prof.value.size() < prof.narg) {
            throw parse_error(
                error_bio(
                    code::OPT_PARSE | code::INS_NARG,
                    input_viewer.count_iterated(),
                    prof
                )
            );
        }
        
    }

    inline void fetch_posarg (
        std::vector<const char*>::const_iterator& dump_iter,
        std::vector<const char*>::const_iterator& dump_end,
        profile& prof,
        std::size_t dump_size
    ) {
        std::size_t narg = prof.narg;
        ++prof.times_called;
        while(dump_iter != dump_end) {
            if((narg == 0) && prof.is_strict) break;
            prof.value.push_back(*dump_iter);
            --narg;
            ++dump_iter;
        }
        if((signed)narg > 0) {
            std::cout << "not enough args" << std::endl;
            throw parse_error(
                error_bio(
                    code::POSARG_PARSE | code::INS_NARG,
                    (dump_size - std::distance(dump_iter, dump_end)),
                    prof
                )
            );
        }
    }

    inline void parse_opt(
        iterator_viewer<const char*>& argv_iter,
        std::vector<const char*>& dump
    ) {
        opt_map::iterator it;
        const char* token;
        char c;
        std::uint64_t conflicts = 0;

        while(!argv_iter.end_reached()) {
            token = *argv_iter;
            if(token[0] == '-') {
                if(((c = token[1]) == '\0') || is_num(c)) {
                    ++argv_iter;
                    dump.push_back(token);
                }
                if((it = map.find(token)) == map.end()) {
                    throw parse_error(
                        error_bio(
                            code::OPT_PARSE | code::UNKNOWN_TOKEN,
                            argv_iter.count_iterated()
                        )
                    );
                }
                handle_opt(argv_iter, *it->second, conflicts);

            } else { 
                dump.push_back(token);
                ++argv_iter;
            }
        }
    }

    void parse_posarg(
        std::vector<const char*>::const_iterator dump_iter,
        std::vector<const char*>::const_iterator dump_end
    ) {
        auto start = std::chrono::high_resolution_clock::now();
        auto posarg_iter = posargs.begin();
        auto posarg_end = posargs.end();
        std::size_t size = std::distance(dump_iter, dump_end);
        
        while((dump_iter != dump_end) && (posarg_iter != posarg_end)) {
            fetch_posarg(dump_iter, dump_end, *(*(posarg_iter++)), size);
        }
        
        if(dump_iter != dump_end) { 
            std::cout << "Dump iter != dump_end" << std::endl;
            throw parse_error(
                error_bio(code::POSARG_PARSE | code::UNKNOWN_TOKEN,
                (size - std::distance(dump_iter, dump_end)))
            );
        }

        if(posarg_iter != posarg_end) {
            std::cout << "posarg iter != posarg_end" << std::endl;
            throw parse_error(
                error_bio(code::POSARG_PARSE | code::UNCALLED_REQ, 0, **posarg_iter)
            );
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Estimated : " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microsecs" << std::endl;
    }

    void finalize() {
        for(auto& prof : profiles) {
            if(prof.is_required && !prof.times_called) {
                throw parse_error(
                    error_bio(
                        code::UNCALLED_REQ | code::FINALIZE,
                        -1,
                        prof
                    )
                );
            }
        }

        for(auto& prof : profiles) {
            if(prof.times_called) prof.callback(prof);
        }
    }

    /*
        Error message should be showed
        because add() in this context normally would be in "initialize" stage of program
        which is the devs responsibility
        I didn't want short error messages, might cause confusion
    */
    public : 

    void add(profile& temp) {
        bool lname_empty = temp.lname.empty();
        bool sname_empty = temp.sname.empty();
        profiles.push_back(std::move(temp));
        profile& prof = profiles.back(); 

        if(lname_empty && sname_empty) {
            // Throw error to catch. so no possible bug could continue
            throw std::invalid_argument("Can't add profile with completely empty identifier, check your input !");
        }

        if(!lname_empty) {
            if(prof.lname[0] == flag_pref) {
                if(!valid_long_opt_name(prof.lname.c_str())) {
                    throw std::invalid_argument(
                        "Invalid identifier for \"" + prof.lname 
                        + "\" for having an invalid format\n\tIt must follow pattern of \"--[a-zA-Z0-9_]+\""
                    );
                }
            } else {
                if(!sname_empty) {
                    throw std::invalid_argument("Invalid identifier for \"" + prof.lname
                        + "\" Detected as posarg\n\tbut have a sname of " + prof.sname);
                }
                if(!valid_posarg_name(prof.lname.c_str())) {
                    throw std::invalid_argument("Invalid identifier for \"" + prof.lname 
                        + "\" for having an invalid format\n\tIt must follow pattern of \"[a-zA-Z0-9_]+\"");
                }
                posargs.push_back(&prof);
            }

            if(!map.emplace(prof.lname.c_str(), &prof).second) {
                throw std::invalid_argument("Invalid identifer for \"" + prof.lname + "\" as it is already existed in map");
            }
        }

        if(!sname_empty) {
            if(!valid_short_opt_name(prof.sname.c_str())) {
                throw std::invalid_argument("Invalid identifier for \"" + prof.sname
                    + "\" For having an invalid format\n\tIt must follow pattern of \"-[a-zA-Z0-9]\"");
            }

            if(!map.emplace(prof.sname.c_str(), &prof).second) {
                throw std::invalid_argument("Invalid identifier for \"" + prof.sname + "\" as it is already existed in map");
            }
        }
    }
    
    bool valid_long_opt_name(const char* name)
    {   
        if(name == nullptr) return false;
        if(
            (name[0] != flag_pref) &&
            (name[1] != flag_pref) && 
            (name[2] == '\0')
        ) return false;
        char c;
        name += 2;
        while((c = *name++)) {
            if(!identifier_char_table[c]) return false;
        }
        return true;
    }

    bool valid_short_opt_name(const char* name)
    {
        if(name == nullptr) return false;
        return (
            (name[0] == flag_pref) &&
            (identifier_char_table[name[1]]) &&
            (name[2] == '\0')
        );
    }

    bool valid_posarg_name(const char* name)
    {
        if(name == nullptr) return false;
        if(!name[0]) return false; // if not
        while(name[0] != '\0') {
            if(!identifier_char_table[name[0]]) return false;
            name++;
        }
        return true;
    }

    void map_out() {
        std::cout << "== Profiles ==\n{ ";
        for(auto& prof : profiles) {
            std::cout << "\'" << (prof.lname.empty() ? prof.sname : prof.lname) << "\', ";
        }
        std::cout << "}\n== Map == \n";
        for(auto& pair : map) {
            std::cout << "{ \"" << pair.first << "\" : " << pair.second << " }\n";
        }
        std::cout << std::flush;
    }

    profile* get(const char* name) {
        auto it = map.find(name);
        if(it == map.end()) return nullptr;
        return it->second;
    }

    std::function<void(const char**, int)>& add_subparser(const char* name){
        auto res = subparser.emplace(name, [](const char** _, int a){});
        if(!res.second) {
            throw std::invalid_argument("Name for subparser already exist");
        }
        return res.first->second;
    }

    void parse(const char** argv, std::size_t argc) {
        const char* t = nullptr;
        std::size_t real_argc = 0;
        sbc_map::iterator it;
        for(int i = 0; i < argc; i++, real_argc++) {
            if((t = argv[i])[0] == flag_pref) continue;
            else if ((it = subparser.find(t)) != subparser.end()) break;
        }
        std::cout << "argc : " << argc << "\nreal_argc : " << real_argc << std::endl;
        iterator_viewer<const char*> argv_iter(argv, real_argc);
        std::vector<const char*> dump;
        dump.reserve(argc);
        parse_opt(argv_iter, dump);
        vec_map(dump);
        parse_posarg(dump.cbegin(), dump.cend());
        finalize();

        if(real_argc != argc) {
            it->second(argv + real_argc, argc - real_argc);
        }
    }

    void assign(std::function<void(const char**, int)>& func) {
        func = [this](const char** argv, int argc) {
            parse(++argv, --argc);
        };
    }
};
}

void print_bit() {
    std::bitset<32> bits;
    bits = parser::code::CALL_LIMIT;
    std::cout << "CALL_LIMIT    : " << bits.to_string() << "\n";
    bits = parser::code::INS_NARG;
    std::cout << "INS_NARG      : " << bits.to_string() << "\n";
    bits = parser::code::UNCALLED_REQ;
    std::cout << "UNCALLED_REQ  : " << bits.to_string() << "\n";
    bits = parser::code::UNKNOWN_TOKEN;
    std::cout << "UNKNOWN_TOKEN : " << bits.to_string() << "\n";
    bits = parser::code::CONFLICT;
    std::cout << "CONFLICT      : " << bits.to_string() << "\n";
    bits = parser::code::FROM_MASK;
    std::cout << "FROM_MASK     : " << bits.to_string() << "\n";
    bits = parser::code::TYPE_MASK;
    std::cout << "TYPE_MASK     : " << bits.to_string() << "\n";

    bits = (parser::code::INS_NARG | parser::code::POSARG_PARSE);
    std::cout << "EXAMPLE       : " << bits.to_string() << "\n";

}
