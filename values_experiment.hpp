#pragma once

#include <cstdint>
#include <array>
#include <iostream>
#include <variant>
#include <type_traits>
#include <span>
#include <functional>
#include <bit>

#include "commons.hpp"
#include "exceptions.hpp"
#include "flagging.hpp"

namespace sp {
namespace values {

/*
== NOTE ==
Typecode of k.*Arr
internally only means that value may
be inserted with tokens until it denies.

essentially in BoundValue context,
references (kInt, kDob, kStr) does met
the criteria above, with just a little specialty
*/

namespace type_code {
	class Tcode : public sp::flagging::BaseFlag<std::uint8_t, Tcode> { using BaseFlag::BaseFlag; };

	constexpr Tcode ref_category(0b1 << 0);
	constexpr Tcode arr_category(0b1 << 1);
	constexpr Tcode category_fields(0b1111);
	constexpr int field_size = std::popcount(category_fields.value());

	constexpr Tcode kInt = Tcode(0b1 << field_size) | ref_category;
	constexpr Tcode kDob = Tcode(0b10 << field_size) | ref_category;
	constexpr Tcode kStr = Tcode(0b100 << field_size) | ref_category;

	constexpr Tcode kRangedArr = Tcode(0b1 << field_size) |  arr_category;
	constexpr Tcode kDynamicArr = Tcode(0b10 << field_size) | arr_category;

	constexpr bool is_array(const Tcode& code) noexcept {
		return code.has(arr_category);
	}

	constexpr bool is_ref(const Tcode& code) noexcept {
		return code.has(ref_category);
	}

	const char* code_to_str(const Tcode& code) noexcept {
		switch(code.value()){
			case kInt.value() : return "<INT_REF>";
			case kDob.value() : return "<DOUBLE_REF>";
			case kStr.value() : return "<STRING_REF>";
			case kRangedArr.value() : return "<RANGED_ARRAY>";
			case kDynamicArr.value() : return "<DYNAMIC_ARRAY>";
			default : return "<UNKNOWN_TCODE>";
		}
	}
}

constexpr bool is_arr_ctgry(const type_code::Tcode& code) noexcept {
	return code.has(type_code::arr_category);
}

constexpr bool is_ref_ctgry(const type_code::Tcode& code) noexcept {
	return code.has(type_code::ref_category);
}

template <typename T, typename... Ts>
struct is_within_variant {
	static constexpr bool value = false;
};

template <typename T, typename... Ts>
struct is_within_variant<T, std::variant<Ts...>> {
	static constexpr bool value = std::disjunction<std::is_same<T, Ts>...>::value;
};




struct TrackingSpan {
	ArrT viewer;
	std::size_t curr_idx = 0;
	TrackingSpan(const ArrT& view) : viewer(view) {}
	template <typename T>
	bool push_back(const T& val) {
		if(curr_idx >= viewer.size())
			return false;
		viewer[curr_idx++] = val;
		return true;
	}

	template <typename T>
	bool push_back(T&& val) {
		if(curr_idx >= viewer.size())
			return false;
		viewer[curr_idx++] = std::move(val);
		return true;
	}

	void track_reset() noexcept { curr_idx = 0; }
};

template <typename T>
struct TrackingReference : public std::reference_wrapper<T> {
	bool filled = false;
	using std::reference_wrapper<T>::get;
	bool insert(const T& val) {
		if(filled)
			return false;
		this->get() = val;
		return (filled = true);
	}

	bool insert(T&& val) {
		if(filled)	 
			return false;
		this->get() = std::move(val);
		return (filled = true);
	}

	void track_reset() noexcept { filled = false; }
};

using IntRef = TrackingReference<IntT>;
using DobRef = TrackingReference<DobT>;
using StrRef = TrackingReference<StrT>;

template <typename T>
struct to_ref {
	using type = TrackingReference<T>;	
};

template <typename GetType, typename VariantType>
GetType& ce_get(VariantType& ins, std::string_view error_msg) {  // Custom Error
	if(std::holds_alternative<GetType>(ins))
		return std::get<GetType>(ins);
	else 
		throw std::invalid_argument(("(Discriminator : " + std::to_string(ins.index()) + ") ").append(error_msg));
}

class BoundValue {
	private :

	using ConsumeF = std::function<bool(void* val, type_code::Tcode)>;

	using val_type = std::variant<
		std::monostate,
		IntRef,
		DobRef,
		StrRef,
		TrackingSpan
	>;

	val_type value;

	void reset() {
		// we use value index, because it's not messier
		switch(value.index()) {
			case 0 : return;
			case 1 :
				std::get<std::variant_alternative_t<1, val_type>>(value).track_reset();
				break;

			case 2 :
				std::get<std::variant_alternative_t<2, val_type>>(value).track_reset();
				break;

			case 3 :
				std::get<std::variant_alternative_t<3, val_type>>(value).track_reset();
				break;

			case 4 :
				std::get<std::variant_alternative_t<4, val_type>>(value).track_reset();
				break;
		}
	}

	static bool fill_str(void* var, type_code::Tcode code, BoundValue& ins) {
		if(!var || (code != type_code::kStr))
			throw except::ParseError("fill_str : invalid argument");
		
		return ce_get<StrRef>(ins.value, "fill_str : get failed")
				.insert(*reinterpret_cast<StrT*>(var));
	}

	static bool fill_int(void* var, type_code::Tcode code, BoundValue& ins) {
		if(!var || (code != type_code::kInt))
			throw except::ParseError("fill_int : invalid argument");
		
		return ce_get<IntRef>(ins.value, "fill_int : get failed")
				.insert(*reinterpret_cast<IntT*>(var));
	}

	static bool fill_dob(void* var, type_code::Tcode code, BoundValue& ins) {
		if(!var || (code != type_code::kDob))
			throw except::ParseError("fill_dob : invalid argument");
		
		return ce_get<DobRef>(ins.value, "fill_dob : get failed")
				.insert(*reinterpret_cast<DobT*>(var));
	}

	static bool fill_arr(void* var, type_code::Tcode code, BoundValue& ins) {
		if(!var) 
			throw except::ParseError("fill_arr : \"var\" argument is a nullptr");

		TrackingSpan& arr = 
			ce_get<TrackingSpan>(ins.value, "fill_arr : get failed");

		switch (code.value())
		{
		case type_code::kInt.value() : return arr.push_back(*reinterpret_cast<IntT*>(var));
		case type_code::kDob.value() : return arr.push_back(*reinterpret_cast<DobT*>(var));
		case type_code::kStr.value() : return arr.push_back(*reinterpret_cast<StrT*>(var));
		default: throw except::ParseError(
			(std::string("fill_arr : type_code of ") +
			type_code::code_to_str(code)) +
			" Is unacceptable"
		);
		}
	}

	static bool fill_fail(void*, type_code::Tcode, BoundValue&) { return false; }
	// Boolean value indicate BoundValue is done fetching. not an error
	bool (*fill_method)(void*, type_code::Tcode, BoundValue&) = fill_fail;

	template <typename T>
	void set_fill_method() {
		if constexpr (std::is_same_v<T, IntRef>) 
			fill_method = fill_int;
		else if constexpr (std::is_same_v<T,DobRef>) 
			fill_method = fill_dob;
		else if constexpr (std::is_same_v<T, StrRef>)
			fill_method = fill_str;
		else if constexpr (std::is_same_v<T, TrackingSpan>)
			fill_method = fill_arr;
		else 
			throw except::SetupError("Unsupported type set_fill_method failed");
	}

	template <typename T>
	std::size_t consume_amnt_impl() const noexcept { return 0; }

	template<typename T>
	std::size_t consume_amnt_impl<typename to_ref<T>::type>() const noexcept {
		return 1;
	}

	template <>
	std::size_t consume_amnt_impl<TrackingSpan>() const noexcept {
		return std::get<TrackingSpan>(this->value).viewer.size();
	}

	public :

	auto opc() { // open parsing context
		this->reset();
		return [&](void* val, type_code::Tcode code) -> bool {
			return fill_method(val, code, *this);
		};
	}

	template <typename T>
	typename std::enable_if_t<is_within_variant<typename to_ref<T>::type, val_type>::value, void>
	bind(T& ref) {
		this->value = std::ref(ref);
		set_fill_method<to_ref<T>::type>();
	}

	void bind(ArrT arr) {
		this->value = TrackingSpan(arr);
		set_fill_method<TrackingSpan>();
	}

	std::size_t consume_amnt() const {

	}


	values::type_code::Tcode get_code() {
		return std::visit([](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, IntRef>) return values::type_code::kInt;
			if constexpr (std::is_same_v<T, DobRef>) return values::type_code::kDob;
			if constexpr (std::is_same_v<T, StrRef>) return values::type_code::kStr;
			if constexpr (std::is_same_v<T, TrackingSpan>) return values::type_code::kRangedArr;
			else return values::type_code::Tcode();
		}, this->value);
	}
};

}

using TypeCodeT = values::type_code::Tcode;
const TypeCodeT& codeStr = values::type_code::kStr;
const TypeCodeT& codeInt = values::type_code::kInt;
const TypeCodeT& codeDob = values::type_code::kDob;
const TypeCodeT& codeArr = values::type_code::kRangedArr;
}