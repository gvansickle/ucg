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

#ifndef SRC_LIBEXT_MEMORY_HPP_
#define SRC_LIBEXT_MEMORY_HPP_

#include <config.h>

#include <unistd.h> // For sysconf().

#include <cstdlib>  // For aligned_alloc().

#include "hints.hpp"
#include "integer.hpp"

#include "immintrin.h"

#ifdef HAVE_ALIGNED_ALLOC
// Nothing to do.
#elif HAVE_POSIX_MEMALIGN
// Create a thin wrapper around posix_memalign().
inline void* aligned_alloc(size_t algn, size_t size) ATTR_ALLOC_SIZE(2) ATTR_MALLOC;
inline void* aligned_alloc(size_t algn, size_t size) { void *p=0; posix_memalign(&p, algn, size); return p; };
#else
#error "Could not find aligned memory allocator."
#endif

/**
 * Everything anyone could ever hope for in an aligned memory allocation interface, without all the guff.
 *
 * @note aligned_alloc() is declared in cstdlib, and posix_memalign() is in stdlib.h.  I'm bucking the trend here by
 *       putting this in memory.hpp, but it seems not unreasonable.
 *
 * @returns Pointer to heap-allocated memory, aligned as requested, with an additional 1024 kbits at the end,
 *          so you don't have to worry about a vector read "going off the end" and into the next page (==segfault).
 *          Call std::free() to deallocate the memory.
 */
inline void * overaligned_alloc(std::size_t needed_alignment, std::size_t needed_size) ATTR_ALLOC_SIZE(2) ATTR_MALLOC;
inline void * overaligned_alloc(std::size_t needed_alignment, std::size_t needed_size)
{
	//static long page_size = sysconf(_SC_PAGESIZE);
	constexpr size_t vector_size = 1024/8;

	// aligned_alloc() requires size == a power of 2 of the alignment.
	// Additionally, it requires alignment to be "a valid alignment supported by the implementation", per
	// http://en.cppreference.com/w/cpp/memory/c/aligned_alloc.  Practically, this means that on POSIX systems which
	// implement aligned_alloc() via posix_memalign(), "alignment must be a power of two multiple of sizeof(void*)", per
	// http://pubs.opengroup.org/onlinepubs/9699919799/functions/posix_memalign.html.

	// From the caller's perspective, he/she would also rather not worry about "running off the end" and into the next page (which could be
	// owned by a different process), which could happen with e.g. unchecked but legitimate use of multibyte load and store functions (q.v.
	// SSE etc).  The memory-efficient way to address this would be to check if the end of the requested size is within ??? bytes of the
	// next page, and if so, allocate another page as well.  That would require getting the "???" from the user, though we could probably
	// just use the alignment, or the max vector size supported by the platform.
	// For now, we'll just always over-compensate and allocate the additional page.

	// Here's where we sort all this out:
	if(!is_power_of_2(needed_alignment) || (needed_alignment < sizeof(void*)))
	{
		// needed_alignment is not a power of two multiple of sizof(void*).
		// Technically, sizeof(void*) could be a non-power-of-two here, but then we have much bigger problems.
		throw std::bad_alloc();
	}
	auto requested_size = (needed_size + vector_size + needed_alignment) - ((needed_size + vector_size) & (needed_alignment-1));

	void* retval = aligned_alloc(needed_alignment, requested_size);
	if(retval == nullptr)
	{
		throw std::bad_alloc();
	}

	return retval;
}


#if defined(__SSE4_2__)

template <uint8_t VecSizeBytes>
inline const void* memmem_short_pattern(const void *mem_to_search, size_t memlen, const void *pattern, size_t pattlen) noexcept ATTR_CONST ATTR_ARTIFICIAL;
template <uint8_t VecSizeBytes>
inline const void* memmem_short_pattern(const void *mem_to_search, size_t memlen, const void *pattern, size_t pattlen) noexcept
{
	static_assert(VecSizeBytes == 16, "Only 128-bit vectorization supported");

	constexpr auto vec_size_mask = ~static_cast<decltype(memlen)>(VecSizeBytes-1);

	const char* p1 = (const char *) mem_to_search;
	__m128i frag1;
	__m128i xmm0;

	// Return nullptr if there's no possibility of a match.
	if( pattlen > memlen || !memlen || !pattlen)
	{
		return nullptr;
	}

	assume(pattlen <= 16);

	// Load the pattern.
	const __m128i xmm_patt = _mm_loadu_si128((const __m128i *)pattern);

	while(p1 < (char*)mem_to_search+(memlen&vec_size_mask))
	{
		// Find the start of a match.
		for(; p1 < (char*)mem_to_search+(memlen&vec_size_mask); p1+=VecSizeBytes)
		{
			// Load 16 bytes from mem_to_search.
			frag1 = _mm_lddqu_si128((const __m128i*)p1);

			// Do the search.

			constexpr uint8_t imm8 = _SIDD_BIT_MASK | _SIDD_POSITIVE_POLARITY | _SIDD_CMP_EQUAL_ORDERED | _SIDD_UBYTE_OPS;

			// Returns bitmask of bits set in IntRes2. (SSE4.2)
			/// @note Ordering here is correct: pattern first, string to search second.
			/// @note The multiple _mm_cmpestr?()'s here compile down into a single pcmpestrm insruction,
			/// and serve only to expose the processor flags to the C++ code.  This would probably be easier in
			/// the end if I did it in actual assembly.
			auto cf = _mm_cmpestrc(xmm_patt, pattlen, frag1, 16, imm8);
			xmm0 = _mm_cmpestrm(xmm_patt, pattlen, frag1, 16, imm8);

			if(unlikely(cf))
			{
				// Some bits in xmm0 are set.  Found at least the start of a match, maybe a full match, maybe more than one match.

				// Get the bitmask into a non-SSE register.
				/// @todo This depends on GCC's definition of __m128i as a vector of 2 long longs.
				uint32_t esi = xmm0[0];

				auto fsb = find_first_set_bit(esi);
				if(fsb && ((fsb-1) + pattlen <= 16))
				{
					// Found a full match.
					return reinterpret_cast<const void*>(p1 + (fsb-1));
				}
				else if(fsb)
				{
					// Only found a partial match.
					// Adjust the pointer into the mem_to_search to point at the first matching char,
					// then 'goto' (via break+while) the next for-loop iteration without adding 16.  This will then
					// result in either a full match (since p1 and xmm_pat are now aligned), or no match.
					p1 += fsb-1;
					break;
				}
				// Should never get here.
			}
			// Else no match starts in this 16 bytes, go to the next 16 bytes of the mem_to_search string.
		}
	}

	if(p1 < (const char*)mem_to_search+memlen)
	{
		auto remaining_len = memlen & 0x0F;

		if(remaining_len)
		{
			// There are less than a vector's worth of bytes at the end, but at least one byte.
			if(pattlen > remaining_len)
			{
				// Pattern can't match, not enough bytes left.
				return nullptr;
			}
			else
			{
				frag1 = _mm_lddqu_si128((const __m128i*)p1);
				uint32_t last_match = _mm_cmpestri(xmm_patt, pattlen, frag1, remaining_len,
						_SIDD_LEAST_SIGNIFICANT | _SIDD_POSITIVE_POLARITY | _SIDD_CMP_EQUAL_ORDERED | _SIDD_UBYTE_OPS);

				if(last_match == pattlen)
				{
					// Found a match in there.
					return p1 + last_match;
				}
				else
				{
					return nullptr;
				}
			}
		}
	}

	// Searched the whole string, no matches.
	return nullptr;
}

#endif // __SSE4_2__

#endif /* SRC_LIBEXT_MEMORY_HPP_ */
