/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of UniversalCodeGrep.
 *
 * UniversalCodeGrep is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file */

#ifndef SRC_LIBEXT_INTEGER_HPP_
#define SRC_LIBEXT_INTEGER_HPP_

#include <config.h>

#include <cstdint>

#include "hints.hpp"

// Boost has a template of this nature, but of course more complete.
template <unsigned char NumBits>
struct uint_t
{
	static_assert(NumBits <= 128, "NumBits > 128 not supported");
	using fast = typename uint_t<NumBits+1>::fast;
};
template<> struct uint_t<128> { using fast = unsigned __int128; };
template<> struct uint_t<64> { using fast = uint_fast64_t; };
template<> struct uint_t<32> { using fast = uint_fast32_t; };

/**
 * constexpr function template for determining at compile time if an unsigned value is a power of two or not.
 *
 * @param val
 * @return  true if #val is a power of two.  false otherwise.
 */
template <typename T>
constexpr
	typename std::enable_if<
		std::is_integral<T>::value && std::is_unsigned<T>::value,
	bool>::type is_power_of_2(T val)
{
	// The "val &&" prevents 0 from being incorrectly classified as a power-of-2.
	return val && !(val & (val - 1));
};

/**
 * constexpr function template which clamps integral value #val between [ #lo, #hi ] and returns the result.
 * @note C++17 has something like this, so this should probably go in a "future/algorithms" library.
 */
template <typename T>
constexpr
	typename std::enable_if<
		std::is_integral<T>::value,
	T>::type clamp(const T val, const T lo, const T hi)
{
	return (val > hi) ? hi : ((val < lo) ? lo : val );
};

// Separate declaration here to avoid "attributes are not allowed on a function-definition" error.
constexpr inline uint32_t bswap(uint32_t x) ATTR_CONST ATTR_ARTIFICIAL;
constexpr inline uint32_t bswap(uint32_t x)
{
#if defined(HAVE___BUILTIN_BSWAP32)
	return __builtin_bswap32(x);
#else
	/// @todo create a fallback.
	static_assert(false, "Generic bswap32() not yet implemented.");
#endif
}

constexpr inline uintptr_t count_trailing_zeros(uintptr_t x) ATTR_CONST ATTR_ARTIFICIAL;
constexpr inline uintptr_t count_trailing_zeros(uintptr_t x)
{
#if defined(HAVE___BUILTIN_CTZLL)
	return __builtin_ctzll(x);
#else
	/// @todo create a fallback.
	static_assert(false, "Generic count_trailing_zeros() not yet implemented.");
#endif
}

#endif /* SRC_LIBEXT_INTEGER_HPP_ */
