#pragma once
#include <cstdint>
#include <type_traits>
#include "commons.hpp"

namespace sp {
namespace flagging {


/*

BaseFlag is used to be a flag with type

Something to be used as a flag, but not an integer
every operation is limited to it's type only
unlike integer

Child class are used to define the name,
supposed to be nothing more than that
*/

template <std::integral IntegerType, typename Derived>
class BaseFlag {

	protected :
	IntegerType val = 0;

	public :

	constexpr BaseFlag() {
		static_assert(
			std::derived_from<Derived, BaseFlag<IntegerType, Derived>>,
			"Derived type must derive from BaseFlag<Derived>"
		);
	}
	constexpr BaseFlag(IntegerType new_val) : val(new_val) {
		static_assert(
			std::derived_from<Derived, BaseFlag<IntegerType, Derived>>,
			"Derived type must derive from BaseFlag<Derived>"
		);
	}

	constexpr BaseFlag(const Derived& oth) : val(oth.val) {
		static_assert(
			std::derived_from<Derived, BaseFlag<IntegerType, Derived>>,
			"Derived type must derive from BaseFlag<Derived>"
		);
	}

	constexpr Derived operator&(const Derived& oth) const noexcept {
		return Derived(this->val & oth.val);
	}

	constexpr Derived operator^(const Derived& oth) const noexcept {
		return Derived(this->val ^ oth.val);
	}

	constexpr Derived operator|(const Derived& oth) const noexcept {
		return Derived(this->val | oth.val);
	}

	constexpr Derived operator~() const noexcept {
		return Derived(~this->val);
	}

	constexpr Derived operator<<(IntegerType n) const noexcept {
		return Derived(this->val << n);
	}

	constexpr Derived operator>>(IntegerType n) const noexcept {
		return Derived(this->val >> n);
	}

	constexpr bool operator==(const Derived& oth) const noexcept {
		return (this->val == oth.val);
	}

	constexpr bool operator!=(const Derived& oth) const noexcept {
		return !(*this == oth);
	}

	constexpr Derived& operator=(const Derived& oth) noexcept {
		this->val = oth.val;
		return *this;
	}

	constexpr bool has(const Derived& oth) const noexcept {
		return ((this->val & oth.val) == oth.val);
	}

	constexpr bool none() const noexcept {
		return (this->val == 0);
	}

	constexpr bool any() const noexcept {
		return (this->val != 0);
	}

	constexpr IntegerType value() const noexcept {
		return this->val;
	}

};


}
}