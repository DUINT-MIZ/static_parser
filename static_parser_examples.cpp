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
        .identifier("--files", nullptr)
        .behavior(parser::behave::IS_STRICT)
        .call_count(1)
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

constexpr auto parser_ins = make_parser(profiles);

int main(int argc, char* argv[]) {
    int integer{0};
    std::array<values::Blob, 2> files_data;
    const char* deint = nullptr;
    AligningData<3> dat;

    auto int_mod_prof = modifiable_profile()
                            .set_value(
                                values::BoundValue()
                                        .bind(integer)
                            )
                            .set_callback(
                                [](static_profile _, modifiable_profile& mod_prof) {
                                    std::cout << "--int called";
                                    if(mod_prof.value.try_get_ptr<int>() != nullptr)
                                        std::cout << " with value : " << *(mod_prof.value.try_get_ptr<int>()) << std::endl;
                                    else 
                                        std::cout << " failed to guess type" << std::endl;
                                }
                            );

    auto files_mod_prof = modifiable_profile()
            .set_value(
                values::BoundValue()
                    .bind(iterator_array<values::Blob>(files_data)))
            .set_callback(
                [](static_profile _, modifiable_profile& mod_prof) {
                    std::cout << "--files called";
                    iterator_array<values::Blob>& val = mod_prof.value.get_array();
                    std::cout << " with value : ";
                    for(auto& blob : val) {
                        std::cout << "\'" << *((const char**)blob.data()) << "\' ";
                    }
                    std::cout << std::endl;
                }
            );

    auto deint_mod_prof = modifiable_profile()
            .set_value(
                values::BoundValue()
                    .bind(deint))
            .set_callback(
                    [](static_profile _, modifiable_profile& mod_prof) {
                        std::cout << "--deint called";
                        if(mod_prof.value.try_get_ptr<const char*>() != nullptr)
                            std::cout << " with value : " << *(mod_prof.value.try_get_ptr<const char*>()) << std::endl;
                        else 
                            std::cout << " failed to guess type" << std::endl;
                    }
                ); 


    parser_ins.align(dat, "--int", int_mod_prof );
    
    parser_ins.align(dat, "--files", files_mod_prof );

    parser_ins.align(dat, "--deint", deint_mod_prof);

    parser_ins.parse<3, DumpCapacity<0>>(dat, ++argv, --argc);
}