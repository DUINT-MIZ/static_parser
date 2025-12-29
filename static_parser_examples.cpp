#include <frozen/unordered_map.h>
#include <frozen/string.h>
#define STATIC_PARSER_NO_HEAP
#include "static_parser.hpp"
#include <array>

constexpr std::array<parser::statis::static_profile, 3> profiles =
    {
        parser::statis::static_profile()
        .identifier("--int", "-i")
        .call_count(1)
        .behavior( parser::behave::IS_STRICT )
        .convert_to(parser::statis::values::TypeCode::INT)
        .set_narg(1)
        .exclude_on(0),

        parser::statis::static_profile()
        .identifier("files", nullptr)
        .behavior(parser::behave::IS_STRICT)
        .set_narg(2)
        .convert_to(parser::statis::values::TypeCode::STRING),

        parser::statis::static_profile()
        .identifier("--deint", "-d")
        .call_count(1)
        .behavior( parser::behave::IS_STRICT )
        .convert_to( parser::statis::values::TypeCode::STRING )
        .set_narg(1)
        .exclude_on(0)
    };

constexpr auto map = frozen::make_unordered_map<frozen::string, const parser::statis::static_profile*>({
    {"--int", &profiles[0]},
    {"-i", &profiles[0]},
    {"files", &profiles[1]},
    {"--deint", &profiles[2]},
    {"-d", &profiles[2]}
});

constexpr parser::statis::Parser<
    parser::statis::IDCount<parser::statis::count_identifier(map)>,
    parser::statis::PosargCount<parser::statis::count_posarg(profiles)>
    > parser_ins(map, profiles);

void help_callback(parser::statis::static_profile static_prof, parser::statis::modifiable_profile& mod_prof) { std::cout << "Hello, world !" << std::endl; }

int main(int argc, char* argv[]) {
    parser::statis::AligningData<3> aligning_data;
    std::array<parser::values::Blob, 2> target_files;
    int target = 0;
    const char* deint_data = "";
    parser_ins.align(
        aligning_data,
        "--int",
        parser::statis::modifiable_profile()
        .set_callback(help_callback)
        .set_value(parser::values::BoundValue().bind(target))
    );

    parser_ins.align(
        aligning_data,
        "files",
        parser::statis::modifiable_profile()
        .set_value(
            parser::values::BoundValue()
            .bind(iterator_array<parser::values::Blob>(target_files))
        )
    );

    parser_ins.align(
        aligning_data,
        "--deint",
        parser::statis::modifiable_profile()
        .set_value(
            parser::values::BoundValue()
            .bind(deint_data)
        )
    );

    std::cout << "Parsing started..." << std::endl;
    parser_ins.parse<3, parser::statis::DumpCapacity<2>>(aligning_data, ++argv, --argc);
    for(auto& blob : target_files) {
        std::cout << "Values : " << *((const char**)blob.data()) << std::endl;
    }
    std::cout << "Values : " << target << std::endl;
}