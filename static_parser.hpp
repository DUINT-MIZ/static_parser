#pragma once

#include <iostream>

#include "commons.hpp"
#include "exceptions.hpp"
#include "utils.hpp"
#include "values.hpp"
#include "profiles.hpp"
#include "mapper.hpp"
#include "parser.hpp"

namespace sp {

// main interface to user
using sp::parser::parse;
using namespace sp;
using snOpt = profiles::snOption;
using dnOpt = profiles::dnOption;
using posArg = profiles::Posarg;
using type_code = values::TypeCode;
using ModProf = profiles::modifiable_profile;
using PointingArr = values::pointing_arr;

template <std::size_t IDCount, std::size_t... Is>
constexpr auto
make_map_pairs(
    std::array<std::pair<profiles::NameType, const profiles::static_profile*>, IDCount>& extracted,
    std::index_sequence<Is...>
) {
    return std::array<std::pair<frozen::string, const profiles::static_profile*>, IDCount>
    {{
        {frozen::string(extracted[Is].first), extracted[Is].second}...
    }};
}

template <std::size_t IDCount>
constexpr
frozen::unordered_map<frozen::string, const profiles::static_profile*, IDCount>
make_map(const std::span<const profiles::static_profile>& profiles) { 
    std::array<std::pair<profiles::NameType, const profiles::static_profile*>, IDCount> extracted{};
    std::size_t curr_idx = 0;
    for(const auto& prof : profiles) {
        if(prof.lname) extracted[curr_idx++] = {prof.lname, &prof};
        if(prof.sname) extracted[curr_idx++] = {prof.sname, &prof};
    }

    if(curr_idx != IDCount) 
        throw except::comtime_except("nullptr in name !");

    return 
    frozen::make_unordered_map<frozen::string, const profiles::static_profile*>(
        make_map_pairs(extracted, std::make_index_sequence<IDCount>{})
    );
}

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount>
struct Context {
    static constexpr std::size_t id_count = IDCount;
    static constexpr std::size_t prof_count = ProfCount;
    static constexpr std::size_t posarg_count = PosargCount;
    mapper::ProfileTable<ProfCount, PosargCount> ptable;
    mapper::Mapper<IDCount> mapper;
    template <profiles::DenotedProfile... Prof>
    constexpr Context(const Prof&... prof)
    : ptable(prof...),
      mapper(make_map<IDCount>(ptable.static_profiles), ptable)
    {}

    profiles::modifiable_profile& match(std::span<profiles::modifiable_profile> mprof, const profiles::NameType& name) const {
        const profiles::static_profile* prof = mapper[name];
        if(!prof) 
            throw std::invalid_argument("Unknown name");
        std::size_t index = ptable.profile_index(prof);
        if(index >= mprof.size())
            throw std::invalid_argument("Can't get profile : index out of range from mprof");
        return mprof[index];
    }

    auto get_index_func() const noexcept {
        return [&](profiles::NameType name) -> NumT {
            const profiles::static_profile* prof = this->mapper[name];
            if(!prof) return -1;
            return this->mapper.profile_index(prof);
        };
    }
};

template <profiles::DenotedProfile... Prof>
constexpr std::size_t count_posargs() noexcept {
    std::size_t dummy = 0;
    std::size_t posarg_count = 0;
    ((Prof::is_posarg_type::value ? ++posarg_count : ++dummy), ...);
    return posarg_count;
}

template <profiles::DenotedProfile... Prof>
constexpr auto make_context(const Prof&... prof) {
    constexpr std::size_t ids = (Prof::id_count + ...);
    constexpr std::size_t posarg_count = count_posargs<Prof...>();
    constexpr std::size_t profile_count = sizeof...(Prof);

    return Context<ids, profile_count, posarg_count>(prof...);
}

struct Request {
    ModProf mprof;
    struct {
        NameType name;
        std::size_t placement_index = 0;
    } request;
    Request(ModProf new_mprof, NameType new_name)
        : mprof(std::move(new_mprof)), request{ new_name } {}
};

template <typename T>
concept IsRequest = std::is_same_v<std::decay_t<T>, Request>;

template <std::size_t ProfCount, std::size_t IDCount>
struct RuntimeContext {
    std::array<sp::ModProf, ProfCount> mprofs{};
    mapper::RuntimeMapper<IDCount> mapper;

    void apply_request(Request& req) {
        if(req.request.placement_index >= ProfCount)
            throw except::SetupError("Request placement index is out of bounds");
        mprofs[req.request.placement_index] = req.mprof;
        std::cerr << "Request (" << req.request.name << ") placement index : " << req.request.placement_index << std::endl;
    }

    template <IsRequest... Req>
    RuntimeContext(
        const mapper::Mapper<IDCount>& smapper,
        Req&&... req
    )
    : mapper(smapper, mprofs)
    {
        std::cerr << "Runtime context initialize (sizeof) : " << sizeof...(Req) << std::endl;
        (apply_request(req), ...);
        mapper.verify();
    }
};

template <typename IndexGetF>
void set_request(const IndexGetF& index_get, Request& req) {
    NumT idx = index_get(req.request.name);
    if(idx < 0)
        throw except::SetupError((std::string("Unknown name of \"") + req.request.name) + "\", in Request");
    req.request.placement_index = idx;
}

template <std::size_t IDCount, std::size_t ProfCount, std::size_t PosargCount, IsRequest... Req>
auto make_rcontext(
    const Context<IDCount, ProfCount, PosargCount>& ctx,
    Req&&... req
) {
    (set_request(ctx.get_index_func(), req), ...);
    std::cerr << "rcontext make" << std::endl;
    return RuntimeContext<ProfCount, IDCount>(ctx.mapper, std::forward<Req>(req)...);
}


}