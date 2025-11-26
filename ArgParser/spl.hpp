/**
 * Standard Parser Library
 * 
 * Library for all parser-specifics definitions and functions
 */

#ifndef MIZ_STANDARD_PARSER_LIBRARY

#define MIZ_STANDARD_PARSER_LIBRARY

#include <stdexcept>
#include <array>
#include <cstdlib>
#include <cstring>
#include <functional>
#include "iterators.hpp"

class St_parser;
class Dy_parser;

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

static constexpr size_t default_array_size = 8;
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
    size_t narg;
    friend class St_parser;
    friend class Dy_parser;
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

    /*
    While in dynamic parser. it's very unlikely to be a valid ptr
    but it's entirely not needed in parser, but more-likely to user define function
    */
    const char* lname = nullptr;
    unsigned char sname = '\0';
};

struct st_profile : public profile {
    private :
    iterator_array<const char*> value;
    friend class St_parser;
    public :

    iterator_viewer<const char*> value_viewer() const { return value.get_viewer(); }
    std::function<void(st_profile&)> callback;
    
    st_profile& set_callback(const std::function<void(st_profile&)>& func) {
        callback = std::move(func);
        return *this;
    }

    st_profile() = default;
    st_profile(st_profile&& oth) = default;
    st_profile& operator=(st_profile&&) = default;

    st_profile(const st_profile&) = delete;
    st_profile& operator=(const st_profile&) = delete;

    // Below function is just for chaining for cleaner code

    st_profile& set_strict(bool enable = true) noexcept {
        is_strict = enable;
        return *this;
    }

    st_profile& set_imme(bool enable = true) noexcept {
        is_immediate = enable;
        return *this;
    }

    st_profile& set_lname(const char* name) noexcept {
        lname = name;
        return *this;
    }

    st_profile& set_sname(unsigned char name) noexcept {
        sname = name;
        return *this;
    }

    st_profile& call_count(int call_count){
        permitted_call_count = call_count;
        return *this;
    }
};

struct dy_profile : public profile {
    private :
    std::vector<const char*> values;
    friend class Dy_parser;
    public :

    iterator_viewer<const char*> value_viewer() {
        iterator_viewer<const char*> res;
        res.assign(values);
        return res;
    }

    dy_profile& reserve(size_t size) {
        values.reserve(size);
        return *this;
    }

    std::function<void(dy_profile&)> callback;
    dy_profile& set_callback(const std::function<void(dy_profile&)>& func) {
        callback = std::move(func);
        return *this;
    }

    dy_profile() = default;
    dy_profile(dy_profile&& oth) = default;
    dy_profile& operator=(dy_profile&&) = default;

    dy_profile(const dy_profile&) = delete;
    dy_profile& operator=(const dy_profile&) = delete;
    // dy_profile

    // Below function is just for chaining for cleaner code

    dy_profile& set_strict(bool enable = true) noexcept {
        is_strict = enable;
        return *this;
    }

    dy_profile& set_imme(bool enable = true) noexcept {
        is_immediate = enable;
        return *this;
    }

    dy_profile& set_lname(const char* name) noexcept {
        lname = name;
        return *this;
    }

    dy_profile& set_sname(unsigned char name) noexcept {
        sname = name;
        return *this;
    }

    dy_profile& call_count(int call_count){
        permitted_call_count = call_count;
        return *this;
    }
};


#endif