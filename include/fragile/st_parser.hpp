
#ifndef MIZ_STACK_PARSER_HPP
#define MIZ_STACK_PARSER_HPP

#include <ArgParser/pssl.hpp>
#include <type_traits>
#include <iostream>
#include "dependencies/frozen/unordered_map.h"
#include "dependencies/frozen/string.h"

#define MAX_ARG 10

/**
 * St_parser, all of container was managed by user. (Trust me it's worth it)
 * for seasoned user :)))
 */

// Profile values was stored outside. in stack


class St_parser {
    private :
    /**
     * long flag = '--'
     * short flag = '-'
     * posarg = no suffixes
     */
    frozen::unordered_map<frozen::string, st_profile*, 3>* lookup;

    iterator_array<st_profile> options;
    iterator_array<st_profile> posargs;

    bool options_storage_is_defined = false;
    bool posarg_storage_is_defined = false;

    
    /**
     * @brief Inserting data according to st_profile::narg
     * 
     * Number of argument stored at least satisfy st_profile::narg.
     * less will result in an error
     */
    void fetch_dat(
        iterator_viewer<const char*>& input_viewer,
        st_profile& prof,
        bool (*check_token)(const char* token) = [](const char* token){ return false; }
    ) const
    {
        
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
                break;
            }

            if(narg_to_parse == 0) {
                if(prof.is_strict) {        
                    break;
                }
            }
            token = *input_viewer.get_val();
            
            if(check_token(token)) {
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
        
    }

    void parse_opt(
        iterator_viewer<const char*>& argv_iter,
        iterator_array<const char*>& dump
    )
    { 
        
        const char* token;
        st_profile* prof = nullptr;
        while(!argv_iter.end_reached())
        {
            token = *argv_iter.get_val();
            if(token[0] == '-')
            {
                if(std::strcmp(token, "--") == 0) {
                    
                    break;
                }
                if(token[1] == '\0') {
                    
                    dump.push_back(token);
                }
                // if((prof = get(token)) == nullptr) throw token_error(std::string("Unknown option of : ") + token);
                
                if(prof->is_immediate) {
                    prof->callback(*prof);
                    std::exit(0);
                }
                ++argv_iter;
                fetch_dat(argv_iter, *prof, [](const char* token){ return (token[0] == '-'); });
                prof->is_called = true;
            }
            else 
            {
                if(dump.full()) throw storage_full("Dump storage full parsing stopped");
                
                dump.push_back(token);
                ++argv_iter;
            }
        }
        
    }

    void parse_posarg(iterator_array<const char*>& dump)
    {   
        
        auto dump_viewer = dump.get_viewer();
        auto posarg_viewer = posargs.get_viewer();
        
        while(!dump_viewer.end_reached())
        {
            if(posarg_viewer.end_reached()) break;
            
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
        
    }

    void helper__finalize1(iterator_array<st_profile>& arr)
    {   
        auto viewer = arr.get_viewer();
        const st_profile* ptr = nullptr;

        while(!viewer.end_reached()){
            ptr = viewer.get_val();
            if(ptr->is_required && !ptr->is_called) {
                throw prof_restriction("A required option was not called");
            }
            ++viewer;
        }
        
        viewer.rewind();

        while(!viewer.end_reached()) {
            ptr = viewer.get_val();
            if(ptr->is_called) ptr->callback(arr[viewer.count_iterated()]);
            ++viewer;
        }
    }

    void finalize() {
        helper__finalize1(options);
        helper__finalize1(posargs);
    }

    void helper_help1(iterator_array<st_profile>& arr) {
        for(st_profile& opt : arr) {
            std::cout << "[" << (opt.is_required ? "!" : "*") << "] ";
            std::cout << "<" <<  opt.narg << "> ";
            if(opt.sname) {
                std::cout << "-" << opt.sname << ", ";
            }
            if(opt.lname) {
                std::cout << opt.lname << " ";
            }
            std::cout << "\n\t" << opt.desc << "\n\n";
        }
    }

    public :

    const char* desc = "";

    void help() {
        std::cout << "[Usage] : [OPTIONS...] ";
        for(st_profile& prof : posargs) {
            std::cout << "<" << prof.lname << "> ";
        }
        std::cout << "\n" << desc << "\n\nOptions : \n";
        helper_help1(options);
        std::cout << "Posargs : \n";
        helper_help1(posargs);
        std::cout << std::endl;
    }

    St_parser() = default;
    

    template <typename... Args>
    void options_assign(Args&&... iter_arr_constructor_arg) {
        options_storage_is_defined = true;
        options.assign(std::forward<Args>(iter_arr_constructor_arg)...);
        std::array<const char*, 0> dummy_cont;
        add_opt<0>(dummy_cont, LongShort, false, 0, "--help", "-h")
            .set_callback(std::bind(&St_parser::help, this))
            .set_imme(true)
            .set_desc("Open this help page");
    }

    template <typename... Args>
    void posargs_assign(Args&&... iter_arr_constructor_arg) {
        posarg_storage_is_defined = true;
        posargs.assign(std::forward<Args>(iter_arr_constructor_arg)...);
    }

    st_profile* get(const std::string_view& name) const noexcept
    {
        auto it = lookup->find(name);
        if(it == lookup->end()) return nullptr;
        return it->second;
    }

    /*
    bool exist(const std::string_view& name) const
    {
        return (lookup.count(name) != 0);
    }    

    st_profile* get(const std::string_view& name) const noexcept
    {
        auto it = lookup.find(name);
        if(it == lookup.end()) return nullptr;
        return it->second;
    }

    template <size_t storage_size>
    st_profile& add_opt(
        std::array<const char*, storage_size>& arr,
        add_type ins_type,
        bool is_required,
        size_t expected_narg,
        const char* lname,
        const char* sname
    )
    {
        
        if(!options_storage_is_defined) throw std::invalid_argument("Can't add option : options storage are not defined");
        if(options.full()) throw storage_full("Can't add option : St_parser main storage full");

        if(!(ins_type & (Short | Long)))
        {
            throw std::invalid_argument("Can't add option : Invalid function argument");
        }
        
        st_profile p;
        p.value.assign<storage_size>(arr);
        p.narg = expected_narg;
        p.is_required = is_required;
        
        options.push_back(std::move(p));
        
        st_profile* ptr = &options[options.size() - 1];


        if(ins_type & Short)
        {   
            
            if(!valid_short_opt_name(sname)) throw std::invalid_argument(std::string("Can't add option : Invalid short option name format, for : ") + sname);            
            auto res_pair = lookup.emplace(sname, ptr);
            if(!res_pair.second) throw std::invalid_argument(std::string("Can't add option : Such a short option name exist for : ") + sname);
            ptr->sname = sname[1];
        }

        if(ins_type & Long) 
        {
            
            if(!valid_long_opt_name(lname)) throw std::invalid_argument(std::string("Can't add option : Invalid long option name format, for : ") + lname);
            auto res_pair = lookup.emplace(lname, ptr);
            if(!res_pair.second) throw std::invalid_argument(std::string("Can't add option : Such a long option name exist for : ") + lname);
            ptr->lname = lname;
        }
        return *ptr;
    }

    template <size_t storage_size>
    st_profile& add_posarg(
        std::array<const char*, storage_size>& arr,
        size_t narg_expected,
        const char* name
    )
    {
        if(!posarg_storage_is_defined) throw std::invalid_argument("Can't add posarg : posarg storage are not defined");
        if(posargs.full()) throw storage_full("Can't add posarg : St_parser main storage full");
        if(narg_expected == 0) {
            throw std::invalid_argument("Can't add posarg : Invalid function argument");
        }

        st_profile p;
        p.value.assign(arr);
        p.is_required = true;
        p.narg = narg_expected;
        posargs.push_back(std::move(p));
        st_profile* ptr = &posargs[posargs.size() - 1];

        if(!valid_posarg_name(name)) throw std::invalid_argument(std::string("Can't add posarg : Invalid posarg name format for : ") + name);
        auto res_pair = lookup.emplace(name, ptr);
        if(!res_pair.second) throw std::invalid_argument(std::string("Can't add posarg : Such a name already exist for : ") + name);
        ptr->lname = name;
        return *ptr;
    }
    */
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


int main() {
    
}

#endif

