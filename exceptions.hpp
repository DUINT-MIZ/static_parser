#pragma once
#include <stdexcept>
#ifndef STATIC_PARSER_NO_HEAP
#include <string>
#endif

namespace sp {

namespace except {



class raw_string_exception : public std::exception {
    private :
    const char* msg;
    public :
    raw_string_exception(const char* err_msg) : std::exception(), msg(err_msg) {}
    const char* what() const noexcept { return msg; }
};

class string_exception : public std::exception {
    private :
    std::string msg;
    public :
    string_exception(const std::string& err_msg) : std::exception(), msg(err_msg) {}

    const char* what() const noexcept { return msg.data(); }
};

class comtime_except : public raw_string_exception {
    public :
    comtime_except(const char* err_msg) : raw_string_exception(err_msg) {};
};

#ifdef STATIC_PARSER_NO_HEAP
class ParseError : public raw_string_exception {
    public :
    ParseError(const char* err_msg) : raw_string_exception(err_msg) {};
};

class SetupError : public raw_string_exception {
    public :
    SetupError(const char* err_msg) : raw_string_exception(err_msg) {}
};

#else

class ParseError : public string_exception {
    public :
    ParseError(const std::string& err_msg) : string_exception(err_msg) {};
    
};

class SetupError : public string_exception {
    public :
    SetupError(const std::string& err_msg) : string_exception(err_msg) {};
};

#endif
}
}