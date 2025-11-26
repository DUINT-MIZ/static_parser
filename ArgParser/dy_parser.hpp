#ifndef MIZ_DYNAMIC_PARSER_HPP
#define MIZ_DYNAMIC_PARSER_HPP

#include "spl.hpp"
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <list>

class Dy_parser {
    private :

    std::unordered_map<std::string, dy_profile*> lookup;
    std::list<dy_profile> options;
    std::list<dy_profile> posargs;
    void fetch_dat(
        iterator_viewer<const char*>& input_viewer,
        dy_profile& prof,
        bool (*check_token)(const char* token) = [](const char* token){ return false; }
    ) const {
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
        auto& value = prof.values;
        size_t narg_to_parse = prof.narg;
        const char* token;
        while(!input_viewer.end_reached())
        {
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
        std::vector<const char*>& dump
    )
    { 
        std::cout << "Start of option parsing... \n";
        const char* token;
        
        dy_profile* prof = nullptr;
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
                    prof->callback(*prof);
                    std::cout << "Exiting...\n";
                    std::exit(0);
                }
                ++argv_iter;
                fetch_dat(argv_iter, *prof, [](const char* token){ return (token[0] == '-'); });
            }
            else 
            {
                std::cout << "Dumped...\n";
                dump.push_back(token);
                ++argv_iter;
            }
        }
        std::cout << "End of option parsing..\n";
    }

    void parse_posarg(std::vector<const char*>& dump)
    {   
        std::cout << "Start Parsing positional arguments..\n";
        auto posarg_it = posargs.begin();
        auto posarg_end = posargs.end();
        iterator_viewer<const char*> dump_viewer(dump);

        while(!dump_viewer.end_reached()) {
            if(posarg_it == posarg_end) break;
            fetch_dat(
                dump_viewer,
                *posarg_it
            );
            ++posarg_it;
        }

        if(!dump_viewer.end_reached()) throw token_error(std::string("Unexpected sequence of token starting from : ") + *dump_viewer.get_val());
        std::cout << "End of positional argument parsing..." << std::endl;
    }

    void helper__finalize1(std::list<dy_profile>& arr)
    {   
        
        for(auto& obj : arr) {
            if(obj.is_required && !obj.is_called) {
                throw prof_restriction("A required option was not called");
            }
        }
        
        for(auto& obj : arr) {
            if(obj.is_called) obj.callback(obj);
        }
    }

    void finalize() {
        helper__finalize1(options);
        helper__finalize1(posargs);
    }

    public :

    Dy_parser() = default;
    
    inline bool exist(const char* name) const noexcept { return (lookup.count(name) != 0); }

    inline dy_profile* get(const char* name) const {
        auto it = lookup.find(name);
        if(it == lookup.end()) return nullptr;
        return it->second;
    }

    dy_profile& add_opt (
        add_type ins_type,
        bool is_required,
        size_t expected_narg,
        const char* lname,
        const char* sname
    ) {
        if(!(ins_type & (Short | Long))) {
            throw std::invalid_argument("Can't add option : Invalid function argument");
        }

        dy_profile* ptr = &options.emplace_back();
        ptr->narg = expected_narg;
        ptr->is_required = is_required;

        if(ins_type & Short) {
            std::cout << "inserting short" << std::endl;
            if(!valid_short_opt_name(sname)) throw std::invalid_argument(std::string("Can't add option : Invalid short name format for : ") + sname);
            auto res_pair = lookup.emplace(sname, ptr); 
            if(!res_pair.second) throw std::invalid_argument(std::string("Can't add option : Such a short option name already exist for : ") + sname);
            ptr->sname = sname[1];
        }

        if(ins_type & Long) {
            std::cout << "inserting long" << std::endl;
            if(!valid_long_opt_name(lname)) throw std::invalid_argument(std::string("Can't add option : Invalid long name format for : ") + lname);
            auto res_pair = lookup.emplace(lname, ptr);
            if(!res_pair.second) throw std::invalid_argument(std::string("Can't add option : Such a long option name already exist for : ") + lname);
            ptr->lname = lname;
        }
        ptr->values.reserve(expected_narg);
        return *ptr;
    }

    dy_profile& add_posarg(size_t expected_narg, const char* name) {
        if(expected_narg == 0) throw std::invalid_argument("Can't add posarg : Invalid function argument");

        dy_profile* ptr = &posargs.emplace_back();
        ptr->narg = expected_narg;
        ptr->is_required = true;

        if(!valid_posarg_name(name)) throw std::invalid_argument(std::string("Can't add posarg : Invalid posarg name format for : ") + name);
        auto res_pair = lookup.emplace(name, ptr);
        if(!res_pair.second) throw std::invalid_argument(std::string("Can't add posarg : Such a posarg name exist for : ") + name);
        ptr->lname = name;
        ptr->values.reserve(expected_narg);
        return *ptr;
    }

    template <typename... Args>
    void parse(Args&&... args){
        iterator_viewer<const char*> argv_iter;
        argv_iter.assign(std::forward<Args>(args)...);
        std::vector<const char*> dump;
        dump.reserve(default_array_size);

        parse_opt(argv_iter, dump);
        parse_posarg(dump);
        finalize();
    }
};

#endif