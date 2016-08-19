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

#include <libext/multiversioning.hpp>
#include <libext/hints.hpp>

#include <cstdint>
#include <immintrin.h>


// Declaration here only so we can apply gcc attributes.
inline uint8_t popcount16(uint16_t bits) noexcept ATTR_CONST /* Doesn't access globals, has no side-effects.*/
	ATTR_ARTIFICIAL; /* == Should appear in debug info even after being inlined. */

#if defined(__POPCNT__) && __POPCNT__==1

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
 * Count the number of bits set in #bits using the Brian Kernighan method (https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan).
 * Iterates once per set bit, i.e. a maximum of 16 times.
 *
 * @note On systems which do not support POPCNT, we can't use the __builtin_popcount() here.  It expands into a function call
 *       to a generic implementation which is much too slow for our needs here.
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

static constexpr size_t f_alignment { alignof(__m128i) };
static constexpr uintptr_t f_alignment_mask { f_alignment-1 };
static_assert(is_power_of_2(f_alignment), "alignof(__m128i) should be a power of 2, but isn't");
static_assert(f_alignment == 16, "alignof(__m128i) should be 16, but isn't");


static inline size_t memcnt_prologue(const char * __restrict__ unaligned_start_ptr, uint16_t num_unaligned_bytes, size_t len, const char searchchar) noexcept
{
	// Load the 16-byte xmm register from the previous 16-byte-aligned address,
	// then ignore any matches before unaligned_start_address and after len.
	// This does a couple of things:
	// 1. Prevents the 16-byte read from spanning a 4KB page boundary into a page that isn't ours.
	// 2. Allows us to do an aligned read (movdqa) vs. an unaligned one (movdqu), maybe gaining us some cycles.

	const __m128i * __restrict__ aligned_pre_start_ptr = reinterpret_cast<const __m128i * __restrict__>(unaligned_start_ptr - (f_alignment - num_unaligned_bytes));

	// The character we're looking for, broadcast to all 16 bytes of the looking_for xmm register.
	// SSE2.
	const __m128i looking_for = _mm_set1_epi8(searchchar);

	// Load an xmm register with 16 aligned bytes.  SSE2, L/Th: 1/0.25-0.5, plus cache effects.
	__m128i prologue_bytes = _mm_load_si128(aligned_pre_start_ptr);

	// Compare the 16 bytes with searchchar.  SSE2, L/Th: 1/0.5.
	// match_bytemask will contain a 0xFF for a matching byte, 0 for a non-matching byte.
	__m128i match_bytemask = _mm_cmpeq_epi8(prologue_bytes, looking_for);

	// Convert the bytemask into a bitmask in the lower 16 bits of match_bitmask.  SSE2, L/Th: 3-1/1
	uint32_t match_bitmask = _mm_movemask_epi8(match_bytemask);

	// The above should never result in more than the bottom 16 bits of match_bitmask being set.
	// Hint this to the compiler.  This prevents gcc from adding an unnecessary movzwl %rNw,%rNd before the popcount16() call.
	assume(match_bitmask <= 0xFFFFU);

	// Mask off the invalid bits of the match_bitmask, i.e. anything before unaligned_start_ptr,
	// and if len < num_unaligned_bytes, anything after min(len,
	match_bitmask &= 0xFFFFU << (f_alignment - num_unaligned_bytes);
	if(len < num_unaligned_bytes)
	{
		match_bitmask &= 0x0FFFFU >> (num_unaligned_bytes - len);
	}

	// Count any searchchars we found.
	return popcount16(match_bitmask);

}

//__attribute__((target("...")))
size_t MULTIVERSION(FileScanner::CountLinesSinceLastMatch)(const char * __restrict__ cbegin,
		const char * __restrict__ cend) noexcept
{
	size_t num_lines_since_last_match = 0;

	const char * __restrict__ last_ptr = cbegin;

	// Calculate the total number of chars we need to search for '\n's in.
	size_t len = cend-cbegin;

	// The byte offset of the first byte of the string within the first aligned block.
	const uint16_t start_unaligned_byte_offset = reinterpret_cast<uintptr_t>(cbegin) & f_alignment_mask;

	// The character we're looking for, broadcast to all 16 bytes of the looking_for xmm register.
	const __m128i looking_for = _mm_set1_epi8('\n');

	//
	// PROLOGUE
	// Handle any unaligned start bytes.
	//
	if(start_unaligned_byte_offset)
	{
		// The offset wasn't 0, so the string does start at an unaligned address.

		// Determine how many unaligned prologue bytes we have.
		// These are the bytes starting at last_ptr up to but not including the byte at the first aligned address.
		const uint16_t num_unaligned_prologue_bytes = f_alignment - start_unaligned_byte_offset;

		// Count the prologue bytes.
		num_lines_since_last_match += memcnt_prologue(cbegin, num_unaligned_prologue_bytes, len, '\n');

		const uint16_t bytes_consumed = std::min(len, (const size_t)num_unaligned_prologue_bytes);

		len -= bytes_consumed;
		last_ptr += bytes_consumed;
	}

	if(len == 0)
	{
		// The string didn't have > sizeof(__m128i) chars in it.
		return num_lines_since_last_match;
	}

	/// @note Whatever hints we try to give the compiler here regarding the alignment of last_ptr (including creating a const __m128i *)
	/// make no difference to the generated assembly.  Which seems to be OK, I don't see any problems with what gets generated.

	//
	// MAIN LOOP
	// Process the buffer in aligned, sizeof(__m128i) chunks.
	//

	// The number of bytes at the end of the string which do not fill a full __m128i (16 bytes).
	const uint16_t num_epilogue_bytes = reinterpret_cast<uintptr_t>(cend) & f_alignment_mask;
	const char * __restrict__ one_past_last_aligned_address = (cend - num_epilogue_bytes);
	if(last_ptr < one_past_last_aligned_address)
	{
		const size_t num_aligned_reads_bytes = one_past_last_aligned_address - last_ptr;

		while(last_ptr < one_past_last_aligned_address)
		{
			// Load an xmm register with 16 aligned bytes.  SSE2, L/Th: 1/0.25-0.5, plus cache effects.
			__m128i substr = _mm_load_si128((const __m128i * __restrict__)last_ptr);

			// Compare the 16 bytes with '\n'.  SSE2, L/Th: 1/0.5.
			// match_bytemask will contain a 0xFF for a matching byte, 0 for a non-matching byte.
			__m128i match_bytemask = _mm_cmpeq_epi8(substr, looking_for);

			// Convert the bytemask into a bitmask in the lower 16 bits of match_bitmask.  SSE2, L/Th: 3-1/1
			uint32_t match_bitmask = _mm_movemask_epi8(match_bytemask);

			// The above should never result in more than the bottom 16 bits of match_bitmask being set.
			// Hint this to the compiler.  This prevents gcc from adding an unnecessary movzwl %rNw,%rNd before the popcount16() call.
			assume(match_bitmask <= 0xFFFF);

			num_lines_since_last_match += popcount16(match_bitmask);
			last_ptr += 16;
		}

		len -= num_aligned_reads_bytes;
	}

	// Hint to gcc that these vars have these properties at this point.
	// This does prevent at least gcc from exploding the loop below into a huge amount
	// of sse code.
	assume(len < 16);

	//
	// EPILOGUE
	// Take care of any left over bytes.  They will start on an aligned boundary (last_ptr will be aligned),
	// but there may be anywhere from 0 to 15 bytes.
	//
	while(len > 0)
	{
		if(*last_ptr == '\n')
		{
			++num_lines_since_last_match;
		}

		++last_ptr;
		--len;
	}

	return num_lines_since_last_match;
}


