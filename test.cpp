#include <ArgParser/st_parser.hpp>
#include <iostream>
#include <array>
#include <chrono>

void map_out(iterator_viewer<const char*> viewer) {
    std::cout << "[ ";
    while(!viewer.end_reached()) {
        std::cout << "\"" << *viewer.get_val() << "\" ";
        ++viewer;
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char const *argv[]){
    St_parser parser;
    std::array<st_profile, 2> options;
    std::array<const char*, 0> dummy_cont;
    parser.options_assign(options);
    parser.add_opt(dummy_cont, Short, false, 0, nullptr, "-s");
    auto start = std::chrono::high_resolution_clock::now();
    parser.parse(++argv, --argc);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Took : " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
    return 0;
}
