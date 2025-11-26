#include "dy_parser.hpp"
#include <iostream>
#include <list>


void map_out(iterator_viewer<const char*> viewer) {
    std::cout << "[ ";
    while(!viewer.end_reached()) {
        std::cout << "\"" << *viewer.get_val() << "\" ";
        ++viewer;
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char const *argv[]){
    
    Dy_parser parser;
    parser.add_opt(Short, false, 3, nullptr, "-a")
        .call_count(2)
        .set_callback([](dy_profile& prof){
            std::cout << "-a was called !" << std::endl;
            map_out(prof.value_viewer());
        });

    parser.add_opt(Long, false, 2, "--say", nullptr)
        .set_strict()
        .set_callback([](dy_profile& prof){
            std::cout << "--say was called !" << std::endl;
            map_out(prof.value_viewer());
        });

    parser.add_posarg(3, "files")
        .set_strict()
        .set_callback([](dy_profile& prof){
            std::cout << "List of files : ";
            map_out(prof.value_viewer());
        });
    
    parser.parse(++argv, --argc);
}