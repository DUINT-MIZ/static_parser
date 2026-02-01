#pragma once
#include "exceptions.hpp"
#include "values_experiment.hpp"
#include "utils.hpp"
#include "commons.hpp"
#include <cstdint>
#include <type_traits>

#include <functional>

namespace sp {
namespace profiles{

using namespace sp;

static constexpr FlagType kRequired = 1 << 0;
static constexpr FlagType kRestricted = 1 << 1;
static constexpr FlagType kImmediate = 1 << 2;

constexpr bool is_required(FlagType flag) { return ((flag & kRequired) != 0); }
constexpr bool is_restricted(FlagType flag) { return ((flag & kRestricted) != 0); }
constexpr bool is_immediate(FlagType flag) { return ((flag & kImmediate) != 0); }

struct static_profile;

struct ConstructingProfile {
    private :
    NameType lname = nullptr;
    NameType sname = nullptr;
    WholeNumT narg = 0;
    NumT positional_order = 0; 
    NumT exclude_point = -1;
    WholeNumT call_limit = 1;
    FlagType behave = 0;
    TypeCodeT convert_code = 0;
    bool posarg = false;

    constexpr void verify() const {
        if(!lname and !sname)
            throw except::comtime_except("Empty name is forbidden");

        if(posarg) {
            if(sname) 
                throw except::comtime_except("Posarg shuldn't not have a short name");
            if(!lname)
                throw except::comtime_except("Empty long name are forbidden on posarg");
            if(!narg)
                throw except::comtime_except("Empty narg are forbidden on posarg");
            if(exclude_point >= 0)
                throw except::comtime_except("Posarg shouldn't have an exclusion point");
            if(not utils::valid_posarg_name(lname))
                throw except::comtime_except("Invalid posarg name format");
        } else {
            if(lname and not utils::valid_long_opt_name(lname, '-'))
                throw except::comtime_except("Invalid long option name format");
            if(sname and not utils::valid_short_opt_name(sname, '-'))
                throw except::comtime_except("Invalid short option name format");
        }

        if(!narg and (!convert_code.none()))
            throw except::comtime_except("No narg specified shouldn't have a non-NONE convert_code");
        else if (narg and (convert_code.none()))
            throw except::comtime_except("narg are specified, convert_code shouldn't be NONE");


        if(values::is_arr_ctgry(convert_code))
            throw except::comtime_except("Typecode ARRAY doesn't specify any type to convert");

        if(!call_limit)
            throw except::comtime_except("Call limit of 0 are forbidden");

    }

    friend static_profile;

    public :

    constexpr ConstructingProfile& identifier(NameType new_lname, NameType new_sname) {
        lname = new_lname;
        sname = new_sname;
        return *this;
    }

    constexpr ConstructingProfile& behavior(FlagType flag) {
        behave |= flag;
        return *this;
    }

    constexpr ConstructingProfile& convert_to(const TypeCodeT& code) {
        convert_code = code;
        return *this;
    }

    constexpr ConstructingProfile& set_nargs(WholeNumT new_narg) {
        narg = new_narg;
        return *this;
    }
    
    constexpr ConstructingProfile& is_posarg(bool v) {
        posarg = v;
        return *this;
    }
    
    constexpr ConstructingProfile& pos_order(NumT order) {
        positional_order = order;
        return *this;
    }

    constexpr ConstructingProfile& limit_call(WholeNumT n) {
        call_limit = n;
        return *this;
    }
    
    constexpr ConstructingProfile& exclude_on(NumT pos) {
        exclude_point = pos;
        return *this;
    }

    constexpr const ConstructingProfile& profile() const noexcept { return *this; }
    constexpr NameType short_name() const noexcept { return sname; }
    constexpr NameType long_name() const noexcept { return lname; }
};


template <typename Derived>
struct BasicOption : protected ConstructingProfile {
    public :

    using is_posarg_type = std::false_type;

    constexpr BasicOption() : ConstructingProfile() { this->is_posarg(false); }

    constexpr Derived& operator()(NameType name) {
        this->identifier(name, this->short_name());
        return static_cast<Derived&>(*this);
    }
 
    constexpr Derived& operator[](NameType name) {
        this->identifier(this->long_name(), name);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& call_lim(WholeNumT n) noexcept {
        this->limit_call(n);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& nargs(WholeNumT n) noexcept {
        this->set_nargs(n);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& immediate() noexcept {
        this->behavior(kImmediate);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& required() noexcept {
        this->behavior(kRequired);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& restricted() noexcept {
        this->behavior(kRestricted);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& convert(const TypeCodeT& code) noexcept {
        this->convert_to(code);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& exclude(NumT n) noexcept {
        this->exclude_on(n);
        return static_cast<Derived&>(*this);
    }

    constexpr const ConstructingProfile& profile() const noexcept { return *this; }
};

template <typename Derived>
struct BasicPosarg : protected ConstructingProfile {
    public :
    using is_posarg_type = std::true_type;

    constexpr BasicPosarg() : ConstructingProfile() { this->is_posarg(true); }

    constexpr Derived& operator()(NameType name) noexcept {
        this->identifier(name, nullptr);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& nargs(WholeNumT n) noexcept {
        this->set_nargs(n);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& required() noexcept {
        this->behavior(kRequired);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& restricted() noexcept {
        this->behavior(kRestricted);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& convert(const TypeCodeT& code) noexcept{
        this->convert_to(code);
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& order(NumT pos) noexcept {
        this->pos_order(pos);
        return static_cast<Derived&>(*this);
    }

    constexpr const ConstructingProfile& profile() const noexcept { return *this; }
};

struct snOption : public BasicOption<snOption> { // sn = singular name
    private :
    bool inserted_id = false;
    public :

    static constexpr int id_count = 1;

    constexpr snOption& operator()(NameType name) {
        if(inserted_id) throw except::comtime_except("Can't insert more name, max is 1 for snOption");
        this->identifier(name, nullptr);
        inserted_id = true;
        return *this;
    }
 
    constexpr snOption& operator[](NameType name) {
        if(inserted_id) throw except::comtime_except("Can't insert more name, max is 1 for snOption");
        this->identifier(nullptr, name);
        inserted_id = true;
        return *this;
    }
};

struct dnOption : public BasicOption<dnOption> { // dn = Dual Name
    public : 
    static constexpr int id_count = 2;
};

struct Posarg : public BasicPosarg<Posarg> {
    public :
    static constexpr int id_count = 1;
};


template <typename T, typename U = std::decay_t<T>>
concept DenotedProfile = 
    (std::derived_from<U, BasicOption<U>> || std::derived_from<U, BasicPosarg<U>>) &&
    requires() {
        {U::id_count} -> std::convertible_to<int>;
};

struct static_profile {
    public :
    const NameType lname = nullptr;
    const NameType sname = nullptr;
    const WholeNumT call_limit = 1;
    const WholeNumT narg = 0;
    const NumT positional_order = 0;
    const FlagType behave = 0;
    const NumT exclude_point = -1;
    const TypeCodeT convert_code = 0;
    const bool is_posarg = false;

    static_profile() = delete;
    constexpr static_profile(const ConstructingProfile& construct_prof)
    :   
        lname(construct_prof.lname),
        sname(construct_prof.sname),  
        call_limit(construct_prof.call_limit),
        narg(construct_prof.narg),
        positional_order(construct_prof.positional_order),
        behave(construct_prof.behave),
        exclude_point(construct_prof.exclude_point),
        convert_code(construct_prof.convert_code),
        is_posarg(construct_prof.posarg)
    {
        construct_prof.verify();
    }

    constexpr static_profile(const static_profile& oth) = default;
};
struct modifiable_profile {
    bool is_called = false;
    WholeNumT call_count = 0;
    WholeNumT fulfilled_args = 0;
    #ifdef STATIC_PARSER_NO_HEAP
    using FunctionType = void(*)(static_profile, modifiable_profile&);
    #else
    using FunctionType = std::function<void(static_profile, modifiable_profile&)>;
    #endif
    FunctionType callback = [](static_profile, modifiable_profile&){};
    values::BoundValue bval;
    WholeNumT call_frequent() const noexcept { return call_count; }
    modifiable_profile& bind(auto var) { bval.bind(var); return *this; }
    modifiable_profile& set_callback(FunctionType&& func) { callback = func; return *this; }
};

const char* get_name(const static_profile& prof) {
    return (prof.lname ? prof.lname : prof.sname);
}

}
}