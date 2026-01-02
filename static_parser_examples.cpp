#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include "static_parser.hpp"
#include <array>

using namespace parser::statis;

static constexpr std::array<static_profile, 3> profiles =
    {
        static_profile()
        .identifier("--int", "-i")
        .call_count(1)
        .behavior( parser::behave::IS_STRICT )
        .convert_to(parser::statis::values::TypeCode::INT)
        .set_narg(1)
        .exclude_on(0),

        static_profile()
        .identifier("files", nullptr)
        .behavior(parser::behave::IS_STRICT)
        .set_narg(2)
        .convert_to(parser::statis::values::TypeCode::STRING),

        static_profile()
        .identifier("--deint", "-d")
        .call_count(1)
        .behavior( parser::behave::IS_STRICT )
        .convert_to( parser::statis::values::TypeCode::STRING )
        .set_narg(1)
        .exclude_on(0)
};

constexpr auto parser_ins = make_parser(
    []() -> const auto& { return profiles; },
    profiles
);

int main(int argc, char* argv[]) {

}