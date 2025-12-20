
#ifndef MIZ_PARSER_UTILS_HPP
#define MIZ_PARSER_UTILS_HPP
#include <cstdint>
#include <array>

namespace parser {
typedef unsigned int uint;
namespace code {
    constexpr uint FINALIZE      = 0b10 << 0;
    constexpr uint OPT_PARSE     = 0b0  << 0;
    constexpr uint POSARG_PARSE  = 0b1  << 0;
    
    constexpr uint UNKNOWN_TOKEN = 0b1  << 2; // Unknown Token
    constexpr uint INS_NARG      = 0b10 << 2; // Insufficient NArg
    constexpr uint UNCALLED_REQ  = 0b11 << 2; // Uncalled Required
    constexpr uint CALL_LIMIT    = 0b1  << 4;
    constexpr uint CONFLICT      = 0b10 << 4;

    constexpr uint TYPE_MASK = (unsigned)(-1) << 2;
    constexpr uint FROM_MASK = 0b11;
}

namespace behave {
    constexpr uint IS_STRICT = 1 << 2;
    constexpr uint IS_REQUIRED = 1 << 1;
    constexpr uint IS_IMMEDIATE = 1 << 0;
};

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


constexpr bool valid_long_opt_name(const char* name, char flag_pref)
{   
    if(name == nullptr) return false;
    if(
        (name[0] != flag_pref) &&
        (name[1] != flag_pref) && 
        (name[2] == '\0')
    ) return false;
    char c = '\0';
    name += 2;
    while((c = *name++)) {
        if(!identifier_char_table[c]) return false;
    }
    return true;
}

constexpr bool valid_short_opt_name(const char* name, char flag_pref)
{
    if(name == nullptr) return false;
    return (
        (name[0] == flag_pref) &&
        (identifier_char_table[name[1]]) &&
        (name[2] == '\0')
    );
}

constexpr bool valid_posarg_name(const char* name)
{
    if(name == nullptr) return false;
    if(!name[0]) return false; // if not
    while(name[0] != '\0') {
        if(!identifier_char_table[name[0]]) return false;
        name++;
    }
    return true;
}

constexpr int find(const char* str, char c) {
    int res = -1;
    char c1 = '\0';
    for(int i = 0; (c1 = str[i]) != '\0'; i++) {
        if(c1 == c) { 
            res = i;
            break;
        }
    }
    return res;
}

constexpr bool is_num(char c) { return (('0' <= c) && (c <= '9')); }

constexpr std::size_t get_strlen(const char* str) {
    std::size_t len = 0;
    while((str++)[0] != 0) { ++len; }
    return len;
}

}
#endif