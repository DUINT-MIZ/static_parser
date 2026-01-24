#pragma once
#include <cstdint>
#include <variant>
#include <utility>
#include <span>


namespace sp {

using NameType = const char*;
using WholeNumT = std::uint32_t;
using NumT = std::int32_t;
using FlagType = std::uint32_t;

using IntT = int;
using DobT = double;
using StrT = const char*;

using Blob = std::variant<std::monostate, IntT, DobT, StrT>;

using ArrT = std::span<Blob>;

using IntRef = std::reference_wrapper<IntT>;
using DobRef = std::reference_wrapper<DobT>;
using StrRef = std::reference_wrapper<StrT>;

}