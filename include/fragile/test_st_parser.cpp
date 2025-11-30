#include "pssl.hpp"
#include <dependencies/frozen/unordered_map.h>
#include <dependencies/frozen/string.h>
#include <array>

static st_profile a;
static st_profile b;

struct st_profile_holder {
    private :
    bool verified;
    st_profile* item;
    friend class St_parser;
    public :
    st_profile_holder() : verified(false), item(nullptr) {}
};

class St_parser {
    private :
    frozen::unordered_map<frozen::string, st_profile_holder*, 3>* a;
    std::array<st_profile*, 3> abc;

    public : 
};

static st_profile_holder c;

constexpr auto map = frozen::make_unordered_map<frozen::string, st_profile_holder*, 3>({
    {"--flag", &c},
    {"--abc", &c},
    {"-c", &c} 
});


static std::array<const char*, 1> cont1;
static std::array<const char*, 3> cont2;
static std::array<const char*, 5> cont3;
static std::array<const char*, 2> cont4;

static std::array<st_profile, 3> profiles = {
    st_profile(cont1)
    .set_lname("--lmao")
    .set_narg(1)
    .set_strict()
    .set_desc("hell yeah"),
    st_profile(cont2)
    .set_narg(2)
    .set_strict(false)
    .set_desc("hell yeah version 2")
    .set_sname('h')
    .set_lname("--hell")
    .set_desc("-h is short for hell yeah !!!"),
    st_profile(cont3)
    .set_narg(4)
    .set_strict(false)
    .set_desc("hell yeah version 3")
    .set_sname('f')
    .set_lname("--clean")
    .set_desc("This API is clean as hell, anyway do you understood it MIZ ?")
};

int main() {
    
}

