#include "static_parser.hpp"
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <array>

static std::array<const char*, 0> help_data;
static fragile::static_profile_context help_context(help_data);
static std::array<const char*, 1> output_data;
static fragile::static_profile_context output_context(output_data);
static std::array<const char*, 2> tfiles_data;
static fragile::static_profile_context tfiles_context(tfiles_data);


static constexpr std::array<const fragile::static_profile, 3> a = {
    fragile::static_profile(help_context)
    .identifier("--help", 'h')
    .behavior(
        fragile::behave::IS_STRICT |
        fragile::behave::IS_IMMEDIATE
    )
    .set_narg(0),

    fragile::static_profile(tfiles_context)
    .identifier("tfiles", '\0')
    .behavior(
        fragile::behave::IS_STRICT | 
        fragile::behave::IS_REQUIRED
    )
    .set_narg(2),

    fragile::static_profile(output_context)
    .identifier("--output", 'o')
    .behavior(
        fragile::behave::IS_STRICT
    )
    .set_narg(1)
};

constexpr auto map = frozen::make_unordered_map<frozen::string, const fragile::static_profile*>({
    {"--help",  &a[0]},
    {"-h", &a[0]},
    {"tfiles", &a[1]},
    {"--output", &a[2]},
    {"-o", &a[2]}
});

constexpr fragile::static_parser<5, 1> parser(map, a);

int main(int argc, char const *argv[])
{
    help_context.func = [](fragile::static_profile _) { std::cout << "Hello !, Help option are called" << std::endl; };
    tfiles_context.func = [](fragile::static_profile prof) {
        auto& values = prof.context->values;
        std::cout << "Tfiles data : ";
        for(auto data : values) {
            std::cout << "\"" << data << "\", ";
        }
        std::cout << std::endl;
    };
    output_context.func = [](fragile::static_profile prof) {
        auto& values = prof.context->values;
        std::cout << "output name : " << values[0] << std::endl;
    };

    parser.parse<2>(++argv, --argc);
    return 0;
}


