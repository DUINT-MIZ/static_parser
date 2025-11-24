#include "spl.hpp"

class Dy_parser {
    private :

    std::unordered_map<std::string, profile*> lookup;
    std::vector<profile> options;
    std::vector<profile> posargs;

    public :

    Dy_parser() {
        options.reserve(default_array_size);
        posargs.reserve(default_array_size);
    }

    profile& add_opt (
        add_type ins_type,
        bool is_required,
        size_t expected_narg,
        const char* lname,
        const char* sname
    ) {
        
    }


};