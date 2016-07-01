/*
 * Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <config.h>

#include "FileScanner.h"

#include <cstdint>   // For uintptr_t.
#include <immintrin.h>

// Declaration here only so we can apply gcc attributes.
inline uint8_t popcount16(uint16_t bits) noexcept __attribute__((const /* Doesn't access globals, has no side-effects.*/,
		artificial /*Should appear in debug info even after being inlined.*/));

#if defined(__POPCNT__) && __POPCNT__==1
#error "TEST: Shouldn't have POPCNT"

inline uint8_t popcount16(uint16_t bits) noexcept
{
	return __builtin_popcount(bits);
}


#else

/**
 * Count the number of bits set in #bits using the Brian Kernighan method (https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan).
 * Iterates once per set bit, i.e. a maximum of 16 times.
 *
 * @param bits  The 16-bit value to count the set bits of.
 * @return The number of bits set in #bits.
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

#endif

static constexpr size_t f_alignment { sizeof(__m128i) };
static constexpr uintptr_t f_alignment_mask { f_alignment-1 };
/// @todo static_assert() that it's a power of 2.

//__attribute__((target("sse4.2")))
size_t FileScanner::CountLinesSinceLastMatch_sse4_2(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
{
	size_t num_lines_since_last_match = 0;

	const char * last_ptr = prev_lineno_search_end;

	// Calculate the total number of chars we need to search for '\n's in.
	size_t len = start_of_current_match-prev_lineno_search_end;

	// The character we're looking for, broadcast to all 16 bytes of the looking_for xmm register.
	const __m128i looking_for = _mm_set1_epi8('\n');

	// Check if we need to handle an unaligned start address.
	if(reinterpret_cast<uintptr_t>(last_ptr) & f_alignment_mask)
	{
		// We do.  Check if we can use a single unaligned load to search the unaligned starting bytes.
		if(len >= f_alignment)
		{
			// We can, the read won't go past the end of the buffer.

			// Load the first unaligned 16 bytes of the passed string.  Note that we won't actually use
			// all of them in the compare; this is just to get the unaligned portion of the buffer out of the way.
			__m128i substr = _mm_loadu_si128((const __m128i*)last_ptr);
			// This is the number of unaligned bytes.
			const int num_bytes_to_search = f_alignment - (reinterpret_cast<uintptr_t>(last_ptr) & f_alignment_mask);

			// Do the match.
			__m128i match_mask = _mm_cmpestrm(substr, num_bytes_to_search, looking_for, 16, _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_BIT_MASK);
			// Get the bottom 32 bits of the match results.  Bits 0-15 tell us if a match happened in the corresponding byte.
			// SSE2, should result in "movd r32, xmm".
			uint32_t match_bitmask = _mm_cvtsi128_si32(match_mask);
			// Count the bits.
			// Using __builtin_popcount() here vs. popcnt intrinsic.
			/// @todo Could we use SSE4.1 _mm_testc_si128() or similar here?
			num_lines_since_last_match += popcount16(match_bitmask);

			// Adjust for the next portion of the counting.
			// Remember, we only searched num_bytes_to_search bytes, not necessarily all 16 bytes we read in.
			last_ptr += num_bytes_to_search;
			len -= num_bytes_to_search;
		}
		else
		{
			// There aren't 16 bytes to load.  Check the unaligned bytes the slow way.
			while((len > 0) && (reinterpret_cast<uintptr_t>(last_ptr) & f_alignment_mask))
			{
				if(*last_ptr == '\n')
				{
					++num_lines_since_last_match;
				}

				++last_ptr;
				--len;
			}
		}
	}

	if(len == 0)
	{
		// The string didn't have >= sizeof(__m128i) chars in it.
		return num_lines_since_last_match;
	}

	for(size_t i = 0; i<len; last_ptr+=16, i += 16)
	{
		int substr_len = len-i < 16 ? len-i : 16;
		__m128i substr = _mm_load_si128((const __m128i*)last_ptr);
		__m128i match_mask = _mm_cmpestrm(substr, substr_len, looking_for, 16, _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_BIT_MASK);
		uint32_t match_bitmask = _mm_cvtsi128_si32(match_mask);
		num_lines_since_last_match += popcount16(match_bitmask);
	}

	return num_lines_since_last_match;
}


