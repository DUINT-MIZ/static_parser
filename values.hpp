#pragma once

#include <variant>
#include <type_traits>
#include <bit>
#include <cstdint>

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
	constexpr Tcode none = Tcode();
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
struct within_variant {
	static constexpr bool value = false;
};

template <typename T, typename... Ts>
struct within_variant<T, std::variant<Ts...>> {
	static constexpr bool value = std::disjunction<std::is_same<Ts, T>...>::value;
};

struct TrackingSpan {
	ArrT viewer;
	std::size_t curr_idx = 0;
	TrackingSpan(const ArrT& view) : viewer(view) {}
	TrackingSpan& operator=(const ArrT& view) {
		viewer = view;
		this->track_reset();
		return *this;
	}
	template <std::size_t N>
	TrackingSpan(std::array<Blob, N>& arr) : viewer(arr) {}

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

	std::size_t consume_amount() const noexcept { return viewer.size(); }
};

template <typename T>
struct TrackingReference {
	bool filled = false;
	std::reference_wrapper<T> ref;
	
	TrackingReference(T& var) : ref(var) {}

	TrackingReference& operator=(T& var) {
		filled = false;
		this->track_reset();
		return *this;
	}

	bool push_back(const T& val) {
		if(filled)
			return false;
		this->ref.get() = val;
		return (filled = true);
	}

	bool push_back(T&& val) {
		if(filled)
			return false;
		this->ref.get() = std::move(val);
		return (filled = true);
	}

	template <typename ParamType>
	bool push_back(const ParamType& _) {
		std::string signature = __PRETTY_FUNCTION__;
		signature = signature.substr(signature.find('['));
		throw except::ParseError("Wrong type : " + signature);
	}

	void track_reset() noexcept { filled = false; }
	std::size_t consume_amount() const noexcept { return 1; }
};

using IntRef = TrackingReference<IntT>;
using DobRef = TrackingReference<DobT>;
using StrRef = TrackingReference<StrT>;

template <typename GetType, typename VariantType>
GetType& ce_get(VariantType& ins, std::string_view error_msg) {  // Custom Error
	if(std::holds_alternative<GetType>(ins))
		return std::get<GetType>(ins);
	else
		throw std::invalid_argument(("(Discriminator : " + std::to_string(ins.index()) + ") ").append(error_msg));
}

class BoundValue {
	private :

	using val_type = std::variant<
		std::monostate,
		IntRef,
		DobRef,
		StrRef,
		TrackingSpan
	>;

	val_type value;

	void reset() {
		std::visit([](auto&& arg){
			using T = std::decay_t<decltype(arg)>;
			if constexpr (!std::is_same_v<T, std::monostate>) {
				arg.track_reset();
			}
		}, this->value);
		
	}

	public :

	auto opc() { // open parsing context
		this->reset();
		return [this](auto&& var) {
			return std::visit([&var](auto&& data) {
				using T = std::decay_t<decltype(data)>;
				if constexpr (std::is_same_v<T, std::monostate>)
					return false;
				else
					return data.push_back(var);
				
			}, this->value);
		};
	}

	template <typename T>
	typename std::enable_if_t<within_variant<std::decay_t<T>, val_type>::value, void>
	bind(T ref) { this->value = ref; }

	std::size_t consume_amnt() const noexcept {
		return std::visit([](auto&& arg) -> std::size_t {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::monostate>)
				return 0;
			else 
				return arg.consume_amount();
		}, this->value);
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
const TypeCodeT& kCodeNone = values::type_code::none;
const TypeCodeT& kCodeInt = values::type_code::kInt;
const TypeCodeT& kCodeDob = values::type_code::kDob;
const TypeCodeT& kCodeStr = values::type_code::kStr;
}
