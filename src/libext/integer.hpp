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
#include <tuple>

#include "hints.hpp"

// Make sure we know our endianness.
#if !defined(__BYTE_ORDER__)
#error "Cannot determine host byte order."
#endif

/**
 * Recursive template for creating an unsigned integral type with at least #NumBits bits.
 * Boost has a more complete template of this nature.
 *
 * Example:
 * @code{.cpp}
 * // I need a type with at least 34 bits:
 * using uint_34 = uint_t<34>::type;
 * // Now use the new type.
 * uint_34 var = 12;
 * [...]
 * @endcode
 */
template <unsigned char NumBits>
struct uint_t
{
	static_assert(NumBits <= 64, "NumBits > 64 not supported");

	/// @name Member Types
	/// @{
	using type = typename uint_t<NumBits+1>::type;
	/// @}

	// The number of bits this type can hold (ala std::bitset).
	static constexpr auto size =  NumBits;
};
//template<> struct uint_t<128> { using type = unsigned __int128; }; ///< @todo x86-64 gcc and clang support this.
//template<> struct uint_t<128> { using type = __m128i; }; ///< @todo Use something like this for platforms without a builtin 128-bit type.
template<> struct uint_t<64> { using type = uint64_t; };
template<> struct uint_t<32> { using type = uint32_t; };
template<> struct uint_t<16> { using type = uint16_t; };
template<> struct uint_t<8> { using type = uint8_t; };

/**
 * Recursive template for creating a signed integral type with at least #NumBits bits.
 * Boost has a more complete template of this nature.
 */
template <unsigned char NumBits>
struct int_t
{
	static_assert(NumBits <= 64, "NumBits > 64 not supported");

	/// @name Member Types
	/// @{
	using type = typename int_t<NumBits+1>::type;
	/// @}

	// The number of bits this type can hold (ala std::bitset).
	static constexpr auto size =  NumBits;
};
//template<> struct int_t<128> { using type = __int128; };  ///< @todo x86-64 gcc and clang support this.
template<> struct int_t<64> { using type = int64_t; };
template<> struct int_t<32> { using type = int32_t; };
template<> struct int_t<16> { using type = int16_t; };
template<> struct int_t<8> { using type = int8_t; };


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
	return static_cast<bool>(val && !(val & (val - static_cast<T>(1))));
};

/**
 * constexpr function template which clamps integral value @c val between [ @c lo, @c hi ] and returns the result.
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


/// Macro for templates which only make sense if they have a specialization.
#define STATIC_ASSERT_NO_SPECIALIZATION(param) \
		/**/ \
		/* @note This tuple business is to get the compiler to dump the name of the type when it errors out.  E.g.: */ \
		/*       "error:'dummy' is not a member of 'std::tuple<long unsigned int>'" */ \
		auto test_var = std::make_tuple(param); \
		static_assert(decltype(test_var)::dummy, "No template specialization for type T.");

template <typename T>
constexpr bool haszero(T x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <typename T>
constexpr bool haszero(T x) noexcept
{
	// Should never get to this unspecialized version.
	STATIC_ASSERT_NO_SPECIALIZATION(x);
	return false;
}

template <> constexpr bool haszero(uint64_t x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <> constexpr bool haszero(uint64_t x) noexcept
{
	// From https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord (public domain)
	return ((x - UINT64_C(0x0101010101010101)) & (~(x) & UINT64_C(0x8080808080808080)));
}

/**
 * Returns the number of nonzero bytes in @c x.
 * @param x
 * @return
 */
template <typename T>
constexpr uint8_t countnonzeros(T x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <typename T>
constexpr uint8_t countnonzeros(T x) noexcept
{
	// Should never get to this unspecialized version.
	STATIC_ASSERT_NO_SPECIALIZATION(x);
	return false;
}

template <> constexpr uint8_t countnonzeros(uint64_t x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <> constexpr uint8_t countnonzeros(uint64_t x) noexcept
{
	return ((x & UINT64_C(0xFF)) > 0)
		+ ((x & UINT64_C(0xFF00)) > 0)
		+ ((x & UINT64_C(0xFF0000)) > 0)
		+ ((x & UINT64_C(0xFF000000)) > 0)
		+ ((x & UINT64_C(0xFF00000000)) > 0)
		+ ((x & UINT64_C(0xFF0000000000)) > 0)
		+ ((x & UINT64_C(0xFF000000000000)) > 0)
		+ ((x & UINT64_C(0xFF00000000000000)) > 0)
		;
}


// Separate declaration here to avoid "attributes are not allowed on a function-definition" error.
template < typename T >
constexpr inline T bswap(T x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template < typename T >
constexpr inline T bswap(T x) noexcept
{
	// Should never get to this unspecialized version.
	STATIC_ASSERT_NO_SPECIALIZATION(x);
}

template <> constexpr inline uint32_t bswap<uint32_t>(uint32_t x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <> constexpr inline uint32_t bswap<uint32_t>(uint32_t x) noexcept
{
#if defined(HAVE___BUILTIN_BSWAP32)
	return __builtin_bswap32(x);
#else
	/// @todo create a fallback.
	static_assert(false, "Generic bswap32() not yet implemented.");
#endif
}

template <> constexpr inline uint64_t bswap<uint64_t>(uint64_t x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <> constexpr inline uint64_t bswap<uint64_t>(uint64_t x) noexcept
{
#if defined(HAVE___BUILTIN_BSWAP64)
	return __builtin_bswap64(x);
#else
	/// @todo create a fallback.
	static_assert(false, "Generic bswap64() not yet implemented.");
#endif
}

/**
 * Portable byte-order conversion of a value from Host order to big-endian.
 *
 * @param x
 * @return
 */
// Separate declaration here to avoid "attributes are not allowed on a function-definition" error.
template <typename T>
constexpr inline T host_to_be(T x) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <typename T>
constexpr inline T host_to_be(T x) noexcept
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return bswap<T>(x);
#else
	return x;
#endif
}

/**
 * Portable byte-order conversion of a value from Host order to little-endian.
 * This overload is for unsigned 32-bit values.
 *
 * @param x
 * @return
 */
// Separate declaration here to avoid "attributes are not allowed on a function-definition" error.
constexpr inline uint32_t host_to_le(uint32_t x) ATTR_CONST ATTR_ARTIFICIAL;
constexpr inline uint32_t host_to_le(uint32_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return x;
#else
	return bswap(x);
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

// Declaration here only so we can apply gcc attributes.
inline uint8_t popcount16(uint16_t bits) noexcept ATTR_CONST /* Doesn't access globals, has no side-effects.*/
	ATTR_ARTIFICIAL; /* Should appear in debug info even after being inlined. */

#if defined(__POPCNT__) && __POPCNT__==1 && defined(HAVE___BUILTIN_POPCOUNT)

/**
 * For systems that support the POPCNT instruction, we can use it through the gcc/clang builtin __builtin_popcount().
 * It inlines nicely into the POPCNT instruction.
 *
 * @param bits
 * @return
 */
inline uint8_t popcount16(uint16_t bits) noexcept
{
	return __builtin_popcount(bits);
}


#else

/**
 * Count the number of bits set in @p bits using the Brian Kernighan method (https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan).
 * Iterates once per set bit, i.e. a maximum of 16 times.
 *
 * @note On systems which do not support POPCNT, we can't use the __builtin_popcount() here.  It expands into a function call
 *       to a generic implementation which is much too slow for our needs here.
 *
 * @param bits  The 16-bit value to count the set bits of.
 * @return The number of bits set in @p bits.
 */
inline uint8_t popcount16(uint16_t bits) noexcept
{
	uint8_t num_set_bits { 0 };

	for(; bits; ++num_set_bits)
	{
		bits &= bits-1;
	}

	return num_set_bits;
}

#endif  // __POPCNT__

/**
 * Find the first set bit in @p bits.
 * @param bits
 * @return  0 if no bits set. 1+bit_index for the first set bit.
 */
inline uint8_t find_first_set_bit(uint32_t bits) noexcept ATTR_CONST ATTR_ARTIFICIAL;
inline uint8_t find_first_set_bit(uint64_t bits) noexcept ATTR_CONST ATTR_ARTIFICIAL;
#if defined(HAVE___BUILTIN_FFS) && defined(HAVE___BUILTIN_FFSL)
// Use gcc's built-in.
inline uint8_t find_first_set_bit(uint32_t bits) noexcept
{
	return __builtin_ffs(bits);
}
inline uint8_t find_first_set_bit(uint64_t bits) noexcept
{
	return __builtin_ffsl(bits);
}
#else
#error "generic find_first_set_bit() not yet implemented."
#endif

#endif /* SRC_LIBEXT_INTEGER_HPP_ */
