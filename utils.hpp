#pragma once
#include <array>
namespace sp {
namespace utils {



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

}
}