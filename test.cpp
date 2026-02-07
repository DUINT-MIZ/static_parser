#include "static_parser.hpp"
#include <iostream>

constexpr auto sctx = sp::make_sctx(
    sp::dnOpt()["-h"]("--help")
        .nargs(0)
        .convert(sp::kCodeNone)
        .immediate(),
    sp::dnOpt()["-o"]("--output")
        .nargs(1)
        .convert(sp::kCodeStr)
        .restricted(),
    sp::Posarg()("files")
        .nargs(1)
        .convert(sp::kCodeStr)
);

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
    std::array<sp::Blob, 4> files_input{};

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