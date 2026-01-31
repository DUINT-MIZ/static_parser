#pragma once
#include <cstdint>
#include <string_view>
#include <cctype>
#include <charconv>
#include "mapper.hpp"
#include "profiles.hpp"
#include "exceptions.hpp"
#include "values_experiment.hpp"

namespace sp {

namespace parser {

using namespace sp;

bool potential_digit(const char* str) {
    int start = 0;
    if((str[0] == '-') or (str[0] == '+')) ++start;
    return std::isdigit(str[start]);
}

void from_chars_result_check(const std::from_chars_result& res, std::string_view input) {
    if(res.ec == std::errc::invalid_argument) 
        throw except::ParseError(std::string("Input : ").append(input) + ", Is not a number");
    
    if(res.ec == std::errc::result_out_of_range) {
        throw except::ParseError(std::string("Input : ").append(input) + ", Is out of range"); 
    }

    if(res.ptr < (input.data() + input.size()))
        throw except::ParseError(std::string("Can't fully convert").append(input) + " To a number");
}

template <typename FillF>
bool convert_and_insert(const FillF& fill, std::string_view input, values::type_code::Tcode code) {
    if(input.empty())
        throw except::ParseError("convert-insert operation failed, input token is empty");
    
    switch(code) {
        case values::type_code::kDob.value() :
            {
                DobT buff = 0;
                from_chars_result_check(
                    std::from_chars(input.data(), input.data() + input.size(), buff),
                    input
                );
                return fill(buff);
            }
            break;

        case codeInt.value() :
            {
                IntT buff = 0;
                from_chars_result_check(
                    std::from_chars(input.data(), input.data() + input.size(), buff),
                    input
                );
                return fill(buff);
            }
            break;

        case codeStr.value() : 
        {
            if(input[input.size()] != '\0')
                throw except::ParseError(std::string("Token : ").append(input) + " Is not null-terminated");
            return fill(input.data());
        }
            break;

        default :
            throw except::ParseError(std::string("Unknown type code of ") + values::type_code::code_to_str(code));
        
    }
}

template <typename ArgGetF>
std::string_view fetch_and_next(
    mapper::FindPair& complete_prof,
    const ArgGetF& get,
    const std::string_view& eq_value,
    bool (*check_token)(const std::string_view&) = [](const std::string_view& _){ return false; }
)
{
    const profiles::static_profile& static_prof = *complete_prof.first;
    profiles::modifiable_profile& mod_prof = *complete_prof.second;
    std::size_t to_parse = static_prof.narg - mod_prof.fulfilled_args;
    std::string_view curr_token;
    auto fill = mod_prof.bval.opc();
    
    if(((signed)to_parse <= 0) && (profiles::is_restricted(static_prof.behave))){
        mod_prof.is_called = true;
        return get();
    }

    if(!eq_value.empty()) {
        if(convert_and_insert(fill, eq_value, static_prof.convert_code)) --to_parse;
        curr_token = get();
        
    } else {
        curr_token = get();
        bool ins_res;
        bool stop_token_criteria_are_met = false;

        long_fetch :
        
        while(to_parse != 0) {
            if(curr_token.empty()) break;
            if((stop_token_criteria_are_met = check_token(curr_token))) break;
            if(
                !(ins_res = convert_and_insert(fill, curr_token, static_prof.convert_code))
            ) break;
            curr_token = get();
            --to_parse;
        }

        if(
            !ins_res 
            or (
                !to_parse 
                and profiles::is_restricted(static_prof.behave)) 
            or curr_token.empty()
            or stop_token_criteria_are_met) {}
        else {
            --to_parse;
            goto long_fetch;
        }
    }

    if((signed)to_parse > 0)
        throw except::ParseError(
            std::string("Insufficient narg for ") + get_name(static_prof)
            + ", still needs " + std::to_string(to_parse)
        );
    mod_prof.is_called = true;
    mod_prof.fulfilled_args += static_prof.narg - (to_parse + mod_prof.fulfilled_args);
    return curr_token;
}

template <typename ArgGetF, typename DumpStoreF, std::size_t IDCount>
void handle_opt(
    mapper::RuntimeMapper<IDCount>& rmap,
    const ArgGetF& get, 
    const DumpStoreF& store
) {
    std::string_view curr_token = get();
    std::string_view eq_value{};
    std::size_t eq_idx = 0;

    while(!curr_token.empty()) {
        if((curr_token[0] != '-') or potential_digit(curr_token.data())) {
            store(curr_token);
            curr_token = get();
            continue;
        }

        if((eq_idx = curr_token.find('=')) != std::string_view::npos) {
            eq_value = curr_token.substr((eq_idx + 1), (curr_token.size() - (eq_idx + 1)));
            curr_token = curr_token.substr(0, eq_idx);
        }

        mapper::FindPair complete_prof = rmap[curr_token];
        if(!complete_prof.first or !complete_prof.second)
            throw except::ParseError(std::string("Unknown flag was passed : ").append(curr_token));
        
        curr_token = fetch_and_next(
            complete_prof, get, eq_value,
            [](const std::string_view& token){ return (token[0] == '-'); }
        );
        if(profiles::is_immediate(complete_prof.first->behave))
            complete_prof.second->callback(*complete_prof.first, *complete_prof.second);
        if(!eq_value.empty())
            eq_value = std::string_view{};
    }
}

template <typename DumpGetF, std::size_t IDCount>
void handle_posarg(const DumpGetF& dump_get, mapper::RuntimeMapper<IDCount>& rmap) {
    std::size_t curr_posarg_order = 0;
    std::string_view curr_token{};
    mapper::FindPair complete_prof;

    while(curr_posarg_order < rmap.existing_posarg()) {
        complete_prof = rmap[mapper::PosargIndex(curr_posarg_order)];
        curr_token = fetch_and_next(complete_prof, dump_get, std::string_view{});
        if(curr_token.empty()) break;
    }

    if(!curr_token.empty())
        throw except::ParseError(std::string("Unexpected dump inputs of ").append(curr_token));
}

template<std::size_t N>
struct DumpSize {};

template <std::size_t IDCount, std::size_t dump_size>
void parse(
    mapper::RuntimeMapper<IDCount>& rmap,
    const char** argv,
    int argc,
    DumpSize<dump_size>
) {
    std::array<std::string_view, dump_size> dump{};
    std::size_t dump_i = 0;
    std::size_t arg_i = 0;
    std::size_t dump_get_i = 0;
    auto arg_get = [&](){
        if(arg_i == argc) return std::string_view{};
        return std::string_view(argv[arg_i++]);
    };

    auto dump_store = [&](const std::string_view& token) -> void {
        if(dump_i == dump_size)
            throw except::ParseError("Dump inputs exceed dump size");
        
        dump[dump_i++] = token;
    };

    auto dump_get = [&]() {
        if(dump_get_i == dump_i) return std::string_view{};
        
        return dump[dump_get_i++];
    };

    handle_opt(rmap, arg_get, dump_store);
    handle_posarg(dump_get, rmap);

    for(std::size_t i{0}; i < rmap.existing_profile(); i++) {
        mapper::FindPair complete_prof = rmap[i];
        if(profiles::is_required(complete_prof.first->behave) and not (complete_prof.second->is_called)) {
            throw except::ParseError(
                (((std::string("A required ")
                + (complete_prof.first->is_posarg ? "posarg" : "option")    
                ) + " of \""
                ) + profiles::get_name(*complete_prof.first)
                ) + "\" was not called"
            );
        }
    }

    for(std::size_t i{0}; i < rmap.existing_profile(); i++) {
        mapper::FindPair complete_prof = rmap[i];
        
        if(complete_prof.second->is_called) 
            complete_prof.second->callback(*complete_prof.first, *complete_prof.second);
    }
}

}
}