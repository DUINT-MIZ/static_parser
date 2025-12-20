
#ifndef MIZ_POLYTYPE_HPP
#define MIZ_POLYTYPE_HPP
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <array>
#include <stdexcept>

namespace parser {
namespace polytype {

enum class type_code : uint32_t {
    STRING,
    INT,
    DOUBLE,
    UNDEFINED
};

using supported_codes = std::integer_sequence<type_code,
    type_code::DOUBLE,
    type_code::INT,
    type_code::STRING
>;

template <typename T>
struct restype_supported 
    : public std::false_type {};

    template <>
struct restype_supported<int>
    : public std::true_type {};

    template <>
struct restype_supported<double>
    : public std::true_type {};

    template <>
struct restype_supported<const char*>
    : public std::true_type {};

template <typename T>
constexpr type_code get_code() { return type_code::UNDEFINED; }
template <> constexpr type_code get_code<int>() { return type_code::INT; }
template <> constexpr type_code get_code<double>() { return type_code::DOUBLE; }
template <> constexpr type_code get_code<const char*>() { return type_code::STRING; }

template <type_code code>
struct type_resolute {
    static_assert(!std::is_same<type_resolute, type_resolute>::value,
        "Resolution fail, specialization for this type_code seems to be non-existent"
    );
};
template <>
struct type_resolute<type_code::STRING> 
    { typedef const char* type; };

template <>
struct type_resolute<type_code::INT> 
    { typedef int type; };

template <>
struct type_resolute<type_code::DOUBLE> 
    { typedef double type; };

template<type_code... Codes>
constexpr auto restype_sizes_impl(std::integer_sequence<type_code, Codes...>) {

    return std::array<std::size_t, sizeof...(Codes)>{
        sizeof(typename type_resolute<Codes>::type)...
    };
}

constexpr auto restype_sizes() {
    return restype_sizes_impl(supported_codes{});
}

constexpr std::size_t restype_max_size() {
    auto sizes = restype_sizes();
    std::size_t max = 0;
    for(std::size_t size : sizes) {
        if(size > max) max = size;
    }
    return max;
}

struct NumberedPtr {
    public : 
    
    type_code code = type_code::UNDEFINED;
    void* ptr = nullptr;

    NumberedPtr() = default;

    template <typename T, typename = std::enable_if_t<restype_supported<T>::value>>
    NumberedPtr(T& var_ref) {
        code = get_code<T>();
        ptr = reinterpret_cast<void*>(&var_ref);
    }

    template <typename T, typename = std::enable_if_t<restype_supported<T>::value>>
    NumberedPtr& operator=(T& var_ref) {
        code = get_code<T>();
        ptr = reinterpret_cast<void*>(&var_ref);
        return *this;
    }

    template <typename T>
    typename std::enable_if<restype_supported<T>::value, T*>::type
    get() {
        if(get_code<T>() == code) {
            return reinterpret_cast<T*>(ptr);
        } else throw std::bad_cast();
    }
};

struct Values {
    alignas(std::max_align_t) char buf[restype_max_size()];
};

}   
}
#endif