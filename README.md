# static_parser
A Command Line Parser that evaluate most of it's required data in compile-time.
No build step required, simply add the headers to your include path

**Compiler used : g++** <br>
**Version Required : c++20** <br>



Usage example :

```cpp
#include <static_parser/static_parser.hpp>
#include <iostream>

// Static Context
constexpr auto sctx = sp::make_sctx(
    sp::dnOpt()["-h"]("--help") // Dual Name Option
        .nargs(0)
        .convert(sp::kCodeNone)
        .immediate(),
    sp::snOpt()["-o"] // Singular Name Option
        .nargs(1)
        .convert(sp::kCodeStr)
        .restricted(),
    sp::Posarg()("files") // Positional Argument
        .nargs(1)
        .convert(sp::kCodeStr)
);
/*
Note :
.nargs() means minimal number of argument required
for this option/posarg.
use .restricted() to prevent fetching argument
more than specified NArgs
*/




// Callback function signature is void(sp::StProf, sp::ModProf&)
void help_msg(sp::StProf _, sp::ModProf& __) {
    std::cout <<
        "Flags : \n" <<
        "-h, --help     = Show this message\n" <<
        "-o, --ouptut   = Output file name" <<
        "files          = input file name" << std::endl;
    std::exit(0);
}


int main(int argc, const char* argv[]) {
    sp::StrT output_name = nullptr;
    std::array<sp::Blob, 4> files_input{}; // Note : Blob is a std::variant

    auto rctx = sp::make_rctx(sctx,
        sp::Request(sp::ModProf().set_callback(help_msg), "-h"),
        sp::Request(sp::ModProf().bind(sp::StrRef(output_name)), "-o"),
        sp::Request(sp::ModProf().bind(sp::TrackSpan(files_input)), "files")
    );
    
    sp::parser::parse(rctx.mapper, ++argv, --argc, sp::parser::DumpSize<4>{});

    std::cout << "Files input : ";
    for(auto& bl : files_input) {
        if(std::holds_alternative<std::monostate>(bl)) {
            std::cout << "<NONE> ";
        } else {
            std::cout << std::get<sp::StrT>(bl) << " ";
        }
    }
        std::cout << std::endl;
    std::cout << "output_name : " << (!output_name ? "<NONE>" : output_name) << std::endl;
    return 0;
}
```