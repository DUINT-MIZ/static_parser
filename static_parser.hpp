#pragma once

#include "commons.hpp"
#include "flagging.hpp"
#include "exceptions.hpp"

#include "utils.hpp"
#include "values_experiment.hpp"
#include "profiles.hpp"
#include "mapper.hpp"
#include "parser.hpp"

#include <frozen/unordered_map.h>
#include <array>

namespace sp {

using ModProf = sp::profiles::modifiable_profile;
using StProf = sp::profiles::static_profile;
using snOpt = sp::profiles::snOption;
using dnOpt = sp::profiles::dnOption;
const TypeCodeT& kCodeNone = values::type_code::none;
const TypeCodeT& kCodeInt = values::type_code::kInt;
const TypeCodeT& kCodeDob = values::type_code::kDob;
const TypeCodeT& kCodeStr = values::type_code::kStr;
using Posarg = sp::profiles::Posarg;

template <profiles::DenotedProfile... Prof>
constexpr std::size_t count_id() {
    return (Prof::id_count + ...);
}

template <std::size_t IDCount, std::size_t... Is>
constexpr auto make_map_pairs(
    const std::array<std::pair<sp::NameType, const profiles::static_profile*>, IDCount>& pairs,
    std::index_sequence<Is...>
) {
    return std::array<std::pair<frozen::string, const profiles::static_profile*>, IDCount>
        {{
            {frozen::string(pairs[Is].first), pairs[Is].second}...
        }};
}

template <std::size_t IDCount>
constexpr auto make_map(std::span<const profiles::static_profile> profs) {
    std::array<std::pair<sp::NameType, const profiles::static_profile*>, IDCount> pairs{};
    std::size_t curr_idx = 0;
    for(const auto& prof : profs) {
        if(prof.lname) pairs[curr_idx++] = {prof.lname, &prof};
        if(prof.sname) pairs[curr_idx++] = {prof.sname, &prof};
    }
    
    return
        frozen::make_unordered_map<frozen::string, const profiles::static_profile*, IDCount>(
            make_map_pairs(pairs, std::make_index_sequence<IDCount>{})
        );
}

template <profiles::DenotedProfile... Prof>
constexpr std::size_t count_posarg() {
    std::size_t res = 0;
    std::size_t dummy = 0;
    ((Prof::is_posarg_type::value ? ++res : ++dummy), ...);
    return res;
}

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount>
struct StaticContext {
    using MapType = frozen::unordered_map<frozen::string, const profiles::static_profile*, IDCount>;
    
    mapper::ProfileTable<ProfCount, PosargCount> ptable;
    mapper::StaticMapper<IDCount> smapper;
    
    template <profiles::DenotedProfile... Prof>
    constexpr StaticContext(Prof&&... prof)
        : ptable(prof...),
          smapper(make_map<IDCount>(ptable.static_profiles), ptable) {}
};


template <profiles::DenotedProfile... Prof>
constexpr auto make_sctx(Prof&&... prof) {
    return
        StaticContext<
            count_id<std::decay_t<Prof>...>(),
            sizeof...(Prof),
            count_posarg<std::decay_t<Prof>...>()
        > (std::forward<Prof>(prof)...);
}

struct Request {
    profiles::modifiable_profile mprof;
    NameType name_request;
    std::size_t placement_index = 0;
    Request(profiles::modifiable_profile new_mprof, NameType new_name_request)
        : mprof(std::move(new_mprof)), name_request(std::move(new_name_request)) {}
};

template <typename T>
concept IsRequest = std::is_same_v<std::decay_t<T>, Request>;

template <std::size_t IDCount, std::size_t ProfCount>
struct RuntimeContext {
    private :
    std::array<profiles::modifiable_profile, ProfCount> mprofs{};

    public :
    mapper::RuntimeMapper<IDCount> mapper;

    template <IsRequest... Req>
    RuntimeContext(const mapper::StaticMapper<IDCount>& smapper, Req&&... req)
     : mapper(smapper, this->mprofs)
    {
        auto apply_request = [&](Request& request) -> void {
            static const profiles::static_profile* ptr = nullptr;
            static std::size_t idx = 0;
            if(!(ptr = smapper[request.name_request]))
                throw except::SetupError(request.name_request + std::string(" is not registered in StaticMapper"));
            if((idx = smapper.profile_index(ptr)) >= ProfCount)
                throw except::SetupError(std::to_string(idx) + " Index is out of bounds");
            mprofs[idx] = std::move(request.mprof);
        };

        (apply_request(req), ...);
        mapper.verify();
    }
};

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount, IsRequest... Req>
RuntimeContext<IDCount, ProfCount>
make_rctx(const StaticContext<IDCount, ProfCount, PosargCount>& sctx, Req&&... req) {
    return RuntimeContext<IDCount, ProfCount>(sctx.smapper, std::forward<Req>(req)...);
}

}