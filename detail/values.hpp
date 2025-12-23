#include "detail/iterators.hpp"
#include <utility>
#include <type_traits>
#include <cstdint>
#include <iostream>

namespace parser {
namespace values {

class BoundValueBadCast : public std::exception {
    public : 
    const char* msg;
    BoundValueBadCast(const char* str) : msg(str) {}
    const char* what() {
        return msg;
    }
};



enum class TypeCode {
    STRING,
    INT,
    DOUBLE,
    UNDEFINED
};

template <typename T>
constexpr TypeCode get_code() { return TypeCode::UNDEFINED; }

template <>
constexpr TypeCode get_code<int>() { return TypeCode::INT; }

template <>
constexpr TypeCode get_code<double>() { return TypeCode::DOUBLE; }

template <>
constexpr TypeCode get_code<const char*>() { return TypeCode::STRING; }

template <typename... Types>
constexpr std::array<std::size_t, sizeof...(Types)> get_sizes() {
    std::array<std::size_t, sizeof...(Types)> res = { sizeof(Types)... };
    return res;
}

template <std::size_t N>
constexpr std::size_t maximum(const std::array<std::size_t, N>& size_list) {
    std::size_t res = 0;
    for(const std::size_t& size : size_list) { if(size > res) res = size; }
    return res;
}

using Blob = std::array<char, maximum(get_sizes<int, double, char>())>;


using ValTagDerive = uint8_t;

enum class ValTag : ValTagDerive {
    ARRAY,
    VAR_REF,
    NONE
};

const char* print_type_code(values::TypeCode code) {
    switch (code)
    {
    case values::TypeCode::INT :
        return "INT";
        break;
    
    case values::TypeCode::DOUBLE :
        return "DOUBLE";
        break;
    
    case values::TypeCode::STRING :
        return "STRING";
        break;

    case values::TypeCode::UNDEFINED :
        return "UNDEFINED";

    default:
        return "Unknown";
        break;
    }
}

struct BoundValue {
    private :

    ValTag tag = ValTag::NONE;
    TypeCode type_code = TypeCode::UNDEFINED; // define only for VAR_REF schema
    union Storage {
        void* pointer_value;
        iterator_array<Blob> array_value;
        constexpr Storage() : pointer_value(nullptr) {}
    } value;

    void move_from(BoundValue&& oth) {
        tag = oth.tag;
        switch (oth.tag)
        {
        case ValTag::ARRAY :
            value.array_value = std::move(oth.value.array_value);
            break;

        case ValTag::VAR_REF :
            value.pointer_value = oth.value.pointer_value;
            type_code = oth.type_code;
            break;
        
        default:
            break;
        }
        oth.reset();
    }
    
    void copy_from(const BoundValue& oth) {
        tag = oth.tag;
        switch (oth.tag)
        {
        case ValTag::ARRAY :
            value.array_value = oth.value.array_value;
            break;

        case ValTag::VAR_REF :
            value.pointer_value = oth.value.pointer_value;
            type_code = oth.type_code;
            break;
        
        default:
            break;
        }
    }

    public :

    BoundValue() = default;
    BoundValue(BoundValue&& oth) { move_from(std::move(oth)); }
    BoundValue(const BoundValue& oth) { copy_from(oth); }
    template <typename T>
    BoundValue(T& var) { bind<T>(var); }
    BoundValue(const iterator_array<Blob>& iterator_arr) { bind(iterator_arr); }

    BoundValue& operator=(BoundValue&& oth) {
        move_from(std::move(oth));
        return *this;
    }

    BoundValue& operator=(const BoundValue& oth) {
        copy_from(oth);
        return *this;
    }

    template <typename T>
    BoundValue& operator=(T& var) {
        bind<T>(var);
        return *this;
    }
    BoundValue& operator=(const iterator_array<Blob>& iterator_arr) {
        bind(iterator_arr);
        return *this;
    }

    template <typename T>
    BoundValue& bind(T& var) {
        static_assert(get_code<T>() != TypeCode::UNDEFINED, "Unknown type code, can't bind...");
        type_code = get_code<T>();
        
        tag = ValTag::VAR_REF;
        value.pointer_value = (void*)&var;
        return *this;
    }

    BoundValue& bind(const iterator_array<Blob>& iterator_arr) {
        tag = ValTag::ARRAY;
        value.array_value = iterator_arr;
        return *this;
    }

    template <typename T>
    T* try_get_ptr() {
        if((get_code<T>() != type_code) ||
            (tag != ValTag::VAR_REF)) return nullptr;

        return ((T*)value.pointer_value);
    }

    iterator_array<Blob>* try_get_array(){ return ((tag == ValTag::ARRAY) ? &value.array_value : nullptr); }

    template <typename T>
    T& get_ptr() {
        if(get_code<T>() != type_code ||
            (tag != ValTag::VAR_REF)) throw BoundValueBadCast("Can't get variable reference, either type_code is wrong. or tag is not VAR_REF");
        return *((T*)value.pointer_value);
    }

    iterator_array<Blob>& get_array() {
        if(tag != ValTag::ARRAY) throw BoundValueBadCast("Can't get array reference, tag is not ARRAY");
        return value.array_value;
    }

    TypeCode get_type_code() const noexcept { return type_code; }
    ValTag get_value_tag() const noexcept { return tag; }

    void* get_raw_ptr() const noexcept {
        if(tag == ValTag::VAR_REF) return value.pointer_value;
        return nullptr;
    }
    
    void reset() {
        tag = ValTag::NONE;
        type_code = TypeCode::UNDEFINED;
        value.pointer_value = nullptr;
    }
};


}
}