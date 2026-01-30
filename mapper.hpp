#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <utility>
#include <frozen/unordered_map.h>
#include <frozen/string.h>
#include <type_traits>

#include "commons.hpp"
#include "exceptions.hpp"
#include "profiles.hpp"

namespace sp {

namespace mapper {

using namespace sp;

template <std::size_t ProfCount, std::size_t PosargCount>
class ProfileTable {
    private :
    std::array<const profiles::static_profile*, PosargCount> posargs{};
    
    public :
    const std::array<profiles::static_profile, ProfCount> static_profiles;
    ProfileTable() = delete;
    template<profiles::DenotedProfile... Prof>
    constexpr ProfileTable(const Prof&... raw_rule)
    : static_profiles({ (raw_rule.profile())... })
    {
        std::size_t curr_posarg_i = 0;
        std::size_t existing_posarg = 0;
        const profiles::static_profile** spot = nullptr;

        for(const auto& prof : static_profiles) {
            if(prof.is_posarg) {
                if(existing_posarg >= PosargCount)
                    throw except::comtime_except("Existing posarg exceed template argument PosargCount");

                if(prof.positional_order < 0)
                    spot = &posargs[curr_posarg_i++];
                else if(prof.positional_order < PosargCount)
                    spot = &posargs[prof.positional_order];
                else
                    throw except::comtime_except("Posarg positional order is out of template argument PosargCount reach");

                if(!(*spot))
                    *spot = &prof;
                else
                    throw except::comtime_except("Posarg positional order is occupied by another posarg");
                ++existing_posarg;
            }
        }

        if(existing_posarg < PosargCount)
            throw except::comtime_except("Existing posarg doesn't match template argument PosargCount");
    }

    constexpr std::size_t profile_index(const profiles::static_profile* prof) const { return prof - &static_profiles[0]; }
    constexpr const std::array<const profiles::static_profile*, PosargCount>& get_posargs() const noexcept { return posargs; }
};

struct PosargIndex {
    std::size_t val = 0;
    PosargIndex(std::size_t i) : val(i) {}
};

template <std::size_t IDCount>
class Mapper {
    private :
    using MapType = frozen::unordered_map<frozen::string, const profiles::static_profile*, IDCount>;
    
    constexpr void verify_relation(const profiles::static_profile* target, profiles::NameType name) {
        auto it = map.find(frozen::string(name));
        if(it == map.end()) 
            throw except::comtime_except("Unknown profile name in map (Forget to register ?)");
        if(it->second != target)
            throw except::comtime_except("Name in map, points to the wrong profile");
    }

    template <std::size_t N>
    constexpr auto get_ptable_posarg(const std::array<const profiles::static_profile*, N>& arr)
    {
        if constexpr  (N == 0) { 
            return {};
        } else {
            return std::span<const profiles::static_profile* const>(arr);
        }
    }

    public :
    const MapType map;
    const std::span<const profiles::static_profile> profiles;
    const std::span<const profiles::static_profile* const> posargs;

    template <std::size_t ProfCount, std::size_t PosargCount>
    constexpr Mapper(
        const MapType& new_map,
        const ProfileTable<ProfCount, PosargCount>& ptable
    ) : map(new_map), profiles(ptable.static_profiles), posargs(get_ptable_posarg(ptable.get_posargs()))
    {
        std::size_t valid_mappings = 0;
        for(const auto& prof : profiles) {
            if(prof.lname) {
                verify_relation(&prof, prof.lname);
                ++valid_mappings;
            }

            if(prof.sname) {
                verify_relation(&prof, prof.sname);
                ++valid_mappings;
            }
        }

        if(valid_mappings < IDCount)
            throw except::comtime_except("Unknown name was assigned to the map");
    }

    const profiles::static_profile* operator[](std::size_t idx) const noexcept {
        if(idx >= profiles.size()) return nullptr;
        return &profiles[idx];
    }

    const profiles::static_profile* operator[](const PosargIndex& posarg_index) const noexcept {
        if(posarg_index.val >= posargs.size()) return nullptr;
        return posargs[posarg_index.val];
    }

    const profiles::static_profile* operator[](const std::string_view& name) const noexcept {
        auto it = map.find(frozen::string(name));
        if(it == map.end()) return nullptr;
        return it->second;
    }

    std::size_t profile_index(const profiles::static_profile* target) const noexcept {
        return (target - &profiles[0]);
    }
};

using FindPair = std::pair<const profiles::static_profile*, profiles::modifiable_profile*>;

template <std::size_t IDCount>
class RuntimeMapper {
    private :
    std::span<profiles::modifiable_profile> mutable_profiles;
    bool is_verified = false;
    public :
    const Mapper<IDCount>& mapper; // const reference in case mapper is compile-time evaluated object

    RuntimeMapper(
        const Mapper<IDCount>& new_mapper,
        const std::span<profiles::modifiable_profile> new_mutable_profiles
    ) : mutable_profiles(new_mutable_profiles), mapper(new_mapper) 
    {}

    FindPair operator[](std::size_t idx) {
        if(not is_verified) throw except::ParseError("RuntimeMapper is not initialized");
        const profiles::static_profile* prof = mapper[idx];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[idx]};
    }

    FindPair operator[](const PosargIndex& posarg_index) {
        if(not is_verified) throw except::ParseError("RuntimeMapper is not initialized");
        const profiles::static_profile* prof = mapper[posarg_index];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[mapper.profile_index(prof)]};
    }

    FindPair operator[](const std::string_view& name) {
        if(not is_verified) throw except::ParseError("RuntimeMapper is not initialized");
        const profiles::static_profile* prof = mapper[name];
        if(!prof) return {nullptr, nullptr};
        return {prof, &mutable_profiles[mapper.profile_index(prof)]};
    }

    std::size_t existing_profile() const noexcept {
        return mapper.profiles.size();
    }

    std::size_t existing_posarg() const noexcept {
        return mapper.posargs.size();
    }

    void verify() {
        if(mutable_profiles.size() != mapper.profiles.size())
            throw std::invalid_argument("mutable profile size doesn't match mapper profile size");
        std::size_t lim = mapper.profiles.size();
        for(std::size_t i = 0; i < lim; i++) {
            const profiles::static_profile& sprof = *mapper[i];
            profiles::modifiable_profile& mprof = mutable_profiles[i];

            if(mprof.bval.get_code().none()) {
                if(mprof.bval.get_code() != sprof.convert_code)    
                    throw std::invalid_argument("BoundValue variable reference type is incompatible with static_profile convert code");
                
                if(sprof.narg > 1)
                    throw std::invalid_argument("static_profile narg more than 1 is incompatible with variable reference BoundValue");
            } else {
                if(mprof.bval.get_value<values::pointing_arr>().viewer.size() < sprof.narg)
                    throw std::invalid_argument("BoundValue array size is less than static_profile narg");
            }
        }
        is_verified = true;
    }
};
}
}