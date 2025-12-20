#include "static_parser.hpp"
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <iostream>

using namespace parser;
constexpr std::array<statis::static_profile, 1> profiles = {
    statis::static_profile()
    .identifier("--help", "-h")
    .behavior(behave::IS_STRICT)
    .convert_to(polytype::type_code::STRING)
    .set_narg(0)
};

constexpr auto map = frozen::make_unordered_map<frozen::string, const statis::static_profile*>({
    {"--help", &profiles[0]},
    {"-h", &profiles[0]}
});

constexpr statis::Parser<statis::IDCount<2>, statis::PosargCount<0>> parser_ins(map, profiles);

int main(int argc, char *argv[])
{   
    
    statis::AligningData<1ul> data;
    const char* str;
    parser_ins.align(
        data,
        "--help",
        statis::modifiable_profile()
        .bind(str)
    );

    parser_ins.parse<1, statis::DumpCapacity<0>>(data, ++argv, --argc);
    
}
