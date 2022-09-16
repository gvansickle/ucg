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

// Std C++.
#include <cstdint>

#include <immintrin.h>

#include <libext/multiversioning.hpp>
#include <libext/hints.hpp>
#include <libext/memory.hpp>


#ifdef __SSE2__
STATIC_MSG("Have SSE2")
#endif
#ifdef __SSE3__
STATIC_MSG("Have SSE3")
#endif
#ifdef __SSSE3__
STATIC_MSG("Have SSSE3")
#endif
#ifdef __SSE4_2__
STATIC_MSG("Have SSE4_2")
#endif
#ifdef __POPCNT__
STATIC_MSG("Have POPCNT")
#endif
#ifdef __AVX__
STATIC_MSG("Have AVX")
#endif



static constexpr size_t f_alignment { alignof(__m128i) };
static constexpr uintptr_t f_alignment_mask { f_alignment-1 };
static_assert(is_power_of_2(f_alignment), "alignof(__m128i) should be a power of 2, but isn't");
static_assert(f_alignment == 16, "alignof(__m128i) should be 16, but isn't");

// Declaration here only so we can apply gcc attributes.
static inline size_t memcnt_prologue(const char * __restrict__ unaligned_start_ptr, uint16_t num_unaligned_bytes, size_t len, char searchchar) noexcept
		ATTR_CONST ATTR_ARTIFICIAL;

static inline size_t memcnt_prologue(const char * __restrict__ unaligned_start_ptr, uint16_t num_unaligned_bytes, size_t len, char searchchar) noexcept
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

#if defined(__SSE4_2__)


const char * MULTIVERSION(FileScanner::find_first_of)(const char * __restrict__ cbegin, size_t len) const noexcept
{
	constexpr auto vec_size_bytes = sizeof(__m128i);
	constexpr auto vec_size_mask = ~static_cast<decltype(len)>(vec_size_bytes-1);

	uint16_t j=0;
	size_t i=0;

	// @note The last vector may spill over the end of the input.  That's ok here, we catch any false
	// hits in the return statements, and our input should have been allocated with at least a vector's worth of
	// padding so we don't hit problems there.
	for(i=0; i < len ; i+=vec_size_bytes)
	{
		// Load an xmm register with 16 unaligned bytes.  SSE3, L/Th: 1/0.25-0.5, plus cache effects.
		// "This intrinsic may perform better than _mm_loadu_si128 when the data crosses a cache line boundary."
		__m128i xmm0 = _mm_lddqu_si128((const __m128i *)(cbegin+i));

		assume(m_end_fpcu_table <= 256);

		for(j=0; j < (m_end_fpcu_table & vec_size_mask); j+=vec_size_bytes)
		{
			// Load our compare-to strings.
			__m128i xmm1_patt = _mm_load_si128((__m128i*)(m_compiled_cu_bitmap+j));
			// Do the "find_first_of()".
			int len_a = ((len-i)>vec_size_bytes) ? vec_size_bytes : (len-i);
			int lsb_set = _mm_cmpestri(xmm1_patt, vec_size_bytes, xmm0, len_a,
					_SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);

			if(lsb_set < 16)
			{
				return std::min(cbegin + i + lsb_set, cbegin + len);
			}
		}

		if(j < m_end_fpcu_table)
		{
			// One partial xmm compare-to register to handle.
			// Load the last partial compare-to string.
			__m128i xmm1_patt = _mm_load_si128((__m128i*)(m_compiled_cu_bitmap+j));
			// Do the "find_first_of()".
			int len_a = ((len-i)>vec_size_bytes) ? vec_size_bytes : (len-i);
			int lsb_set = _mm_cmpestri(xmm1_patt, m_end_fpcu_table & (vec_size_bytes-1), xmm0, len_a,
					_SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);

			if(lsb_set < 16)
			{
				return std::min(cbegin + i + lsb_set, cbegin + len);
			}

		}
	}
	return cbegin+len;
}

const char * MULTIVERSION(FileScanner::find)(const char * __restrict__ cbegin, size_t len) const noexcept
{
	constexpr auto vec_size_bytes = sizeof(__m128i);

	// Broadcast the character we're looking for to all 16 bytes of an xmm register.
	// SSE2.
	const __m128i xmm0 = _mm_set1_epi8(m_compiled_cu_bitmap[0]);
	const __m128i xmm_all_FFs = _mm_set1_epi8(UINT8_C(0xFF));
	for(size_t i=0; i<len; i+=vec_size_bytes)
	{
		// Load an xmm register with 16 unaligned bytes.  SSE3, L/Th: 1/0.25-0.5, plus cache effects.
		__m128i xmm1 = _mm_lddqu_si128((const __m128i *)(cbegin+i));
		// Compare the strings' 16 bytes with the char we seek.  SSE2, L/Th: 1/0.5.
		// match_bytemask will contain a 0xFF for a matching byte, 0 for a non-matching byte.
		__m128i match_bytemask = _mm_cmpeq_epi8(xmm1, xmm0);

		// Did we find any chars?  Check if the resulting match bytemask is all zeros.
		// We do this with an SSE instruction, as we take a performance hit when we convert to "regular" x86-64 instructions.
		// ptest, SSE4.1, L/Th: 2/1.
		if(_mm_test_all_zeros(match_bytemask, xmm_all_FFs) == 0)
		{
			// match_bytemask wasn't all zeros.

			// Convert the bytemask into a bitmask in the lower 16 bits of match_bitmask.  SSE2, L/Th: 3-1/1
			uint32_t match_bitmask = _mm_movemask_epi8(match_bytemask);

			assume(match_bitmask <= 0xFFFFU);

			// Did we find any chars?
			if(match_bitmask > 0)
			{
				// Find the first bit set.
				auto lowest_bit = find_first_set_bit(match_bitmask);
				if(lowest_bit > 0 && (i + lowest_bit - 1 < len))
				{
					return std::min(cbegin + i + lowest_bit - 1, cbegin + len);
				}
			}
		}
	}
	return cbegin+len;
}

#ifdef __POPCNT__ // To eliminate multiple defs.

const char * MULTIVERSION(FileScanner::find_first_in_ranges)(const char * __restrict__ cbegin, size_t len) const noexcept
{
	constexpr auto vec_size_bytes = sizeof(__m128i);
	constexpr auto vec_size_mask = ~static_cast<decltype(len)>(vec_size_bytes-1);

	uint16_t j=0;
	size_t i=0;

	// @note The last vector may spill over the end of the input.  That's ok here, we catch any false
	// hits in the return statements, and our input should have been allocated with at least a vector's worth of
	// padding so we don't hit problems there.
	for(i=0; i < len ; i+=vec_size_bytes)
	{
		// Load an xmm register with 16 unaligned bytes.  SSE3, L/Th: 1/0.25-0.5, plus cache effects.
		// "This intrinsic may perform better than _mm_loadu_si128 when the data crosses a cache line boundary."
		__m128i xmm0 = _mm_lddqu_si128((const __m128i *)(cbegin+i));

		assume(m_end_ranges_table <= 256);

		for(j=0; j < (m_end_ranges_table & vec_size_mask); j+=vec_size_bytes)
		{
			// Load one of our range strings, which are 16-byte aligned.
			__m128i xmm1_ranges = _mm_load_si128((__m128i*)(m_compiled_range_bitmap+j));
			// Do the search for values in the specified ranges.
			int len_a = ((len-i)>vec_size_bytes) ? vec_size_bytes : (len-i);
			int lsb_set = _mm_cmpestri(xmm1_ranges, vec_size_bytes, xmm0, len_a,
					_SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);

			if(lsb_set < 16)
			{
				return std::min(cbegin + i + lsb_set, cbegin + len);
			}
		}

		if(j < m_end_ranges_table)
		{
			// One partial xmm compare-to register to handle.
			// Load the last partial compare-to string.
			__m128i xmm1_ranges = _mm_load_si128((__m128i*)(m_compiled_range_bitmap+j));
			// Do the search for values in the specified ranges.
			int len_a = ((len-i)>vec_size_bytes) ? vec_size_bytes : (len-i);
			int lsb_set = _mm_cmpestri(xmm1_ranges, m_end_ranges_table & (vec_size_bytes-1), xmm0, len_a,
					_SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);

			if(lsb_set < 16)
			{
				return std::min(cbegin + i + lsb_set, cbegin + len);
			}

		}
	}
	return cbegin+len;
}


int FileScanner::LiteralMatch_sse4_2(const char *file_data, size_t file_size, size_t start_offset, size_t *ovector) const noexcept
{
	int rc = 0;
	const char* str_match;
	const size_t bytes_to_search = file_size - start_offset;

	if((m_literal_search_string_len <= 16) && (m_literal_search_string_len>=1))	// Current sse4.2 memmem_short_pattern()
																				// only works for search strings with 1 <= len < 17.
	{
		str_match = (const char*)MV_USE(memmem_short_pattern, ISA_x86_64::SSE4_2)((const void*)(file_data+start_offset), bytes_to_search,
				(const void *)m_literal_search_string.get(), m_literal_search_string_len);
	}
	else
	{
		str_match = (const char*)memmem((const void*)(file_data+start_offset), bytes_to_search,
								(const void *)m_literal_search_string.get(), m_literal_search_string_len);
	}

	if(str_match == nullptr)
	{
		// No match.
		rc = -1; /// @note Both PCRE_ERROR_NOMATCH and PCRE2_ERROR_NOMATCH are both -1.
		ovector[0] = file_size;
		ovector[1] = file_size;
	}
	else
	{
		// Found a match.
		rc = 1;
		ovector[0] = str_match - file_data;
		ovector[1] = ovector[0] + m_literal_search_string_len;
	}

	return rc;
}

#if KEEP_AS_EXAMPLE
template <ISA_x86_64 OuterISA>
static inline auto FileScanner::FindFirstPossibleCodeUnit(const char * __restrict__ cbegin, size_t len) const noexcept
		-> std::enable_if_t<ISAIsSubsetOf(OuterISA, ISA_x86_64::SSE4_2), const char *>
{

}
#endif

#endif // __POPCNT__

#endif
