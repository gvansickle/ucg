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

#include <cstdlib>

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
 * @returns Pointer to heap-allocated memory aligned as requested.  Call std::free() to deallocate it.
 */
inline void * overaligned_alloc(std::size_t needed_alignment, std::size_t needed_size) ATTR_ALLOC_SIZE(2);
inline void * overaligned_alloc(std::size_t needed_alignment, std::size_t needed_size)
{
	static long page_size = sysconf(_SC_PAGESIZE);

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
	auto requested_size = (needed_size + page_size + needed_alignment) - ((needed_size + page_size) & (needed_alignment-1));

	void* retval = aligned_alloc(needed_alignment, requested_size);
	if(retval == nullptr)
	{
		throw std::bad_alloc();
	}

	return retval;
}

#if 1
template <uint8_t VecSizeBytes>
inline const void *memmem(const void *mem_to_search, size_t len1, const void *pattern, size_t len2)
{
	static_assert(VecSizeBytes != 0, "No vectorization of memmem() for this vector size");
	return nullptr;
}
#endif

#if defined(__SSE4_2__)

template<>
inline const void* memmem<16>(const void *mem_to_search, size_t len1, const void *pattern, size_t len2)
{
	size_t idx=0;
	ssize_t ln1= 16, ln2=16;
	ssize_t rcnt1 = len1, rcnt2= len2;
	__m128i *p1 = (__m128i *) mem_to_search;
	__m128i *p2 = (__m128i *) pattern;
	__m128i frag1, frag2;
	int cmp, cmp2, cmp_s;
	__m128i *pt = nullptr;

	// Return nullptr if there's no possibility of a match.
	if( len2 > len1 || !len1) return nullptr;

	// Load the first 1 to 16 bytes of the string to search and the pattern.
	frag1 = _mm_loadu_si128(p1);
	frag2 = _mm_loadu_si128(p2);

	while(rcnt1 > 0)
	{
		cmp_s = _mm_cmpestrs(frag2, (rcnt2>ln2)? ln2: rcnt2, frag1, (rcnt1>ln1)? ln1: rcnt1, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED);
		cmp = _mm_cmpestri(frag2, (rcnt2>ln2)? ln2: rcnt2, frag1, (rcnt1>ln1)? ln1: rcnt1, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ORDERED);
		if(!cmp)
		{
			// we have a partial match that needs further analysis
			if(cmp_s)
			{
				// if we're done with the pattern...
				if(pt)
				{
					idx = (size_t) ((char *) pt - (char *) mem_to_search) ;
				}
				else
				{
					idx = (size_t) ((char *) p1 - (char *) mem_to_search) ;
				}
				return (char *)mem_to_search + idx;
			}
			// we do a round of string compare to verify full match till end of pattern
			if(pt == nullptr)
			{
				pt = p1;
			}
			cmp2 = 16;
			rcnt2 = len2 - 16 - (ssize_t) ((char *)p2-(char *)pattern);
			while( cmp2 == 16 && rcnt2)
			{
				// each 16B frag matches,
				rcnt1 = len1 - 16 -(ssize_t) ((char *)p1-(char *)mem_to_search);
				rcnt2 = len2 - 16 -(ssize_t) ((char *)p2-(char *)pattern);

				if(rcnt1 <= 0 || rcnt2 <= 0 ) break;

				p1 = (__m128i *)(((char *)p1) + 16);
				p2 = (__m128i *)(((char *)p2) + 16);
				frag1 = _mm_loadu_si128(p1);// load up to 16 bytes of fragment
				frag2 = _mm_loadu_si128(p2);// load up to 16 bytes of fragment
				cmp2 = _mm_cmpestri(frag2, (rcnt2>ln2)? ln2: rcnt2, frag1, (rcnt1>ln1)? ln1: rcnt1, 0x18); // lsb, eq each
			};

			if(!rcnt2 || rcnt2 == cmp2)
			{
				idx = (size_t) ((char *) pt - (char *) mem_to_search) ;
				return (char *)mem_to_search + idx;
			}
			else if(rcnt1 <= 0)
			{
				// also cmp2 < 16, non match
				if( cmp2 == 16 && ((rcnt1 + 16) >= (rcnt2+16) ) )
				{
					idx = (int) ((char *) pt - (char *) mem_to_search) ;
					return (char *)mem_to_search + idx;
				}
				else
				{
					return nullptr;
				}
			}
			else
			{
				// Advance the attempted match offset in the mem_to_search by 1
				p1 = (__m128i *)(((char *)pt) + 1);
				rcnt1 = len1 - (ssize_t) ((char *)p1-(char *)mem_to_search);
				pt = NULL;
				p2 = (__m128i *)((char *)pattern) ;
				rcnt2 = len2 - (ssize_t) ((char *)p2-(char *)pattern);
				// Load the next 1-16-byte chunks.
				frag1 = _mm_loadu_si128(p1);
				frag2 = _mm_loadu_si128(p2);
			}
		}
		else
		{
			if(cmp == 16)
			{
				p1 = (__m128i *)(((char *)p1) + 16);
			}
			else
			{
				p1 = (__m128i *)(((char *)p1) + cmp);
			}

			rcnt1 = len1 - (ssize_t) ((char *)p1-(char *)mem_to_search);
			if( pt && cmp )
			{
				pt = NULL;
			}
			// Load next 1-16 bytes from mem_to_search.
			frag1 = _mm_loadu_si128(p1);
		}
	}
	// Didn't find a match.
	return nullptr;
}

#endif // __SSE4_2__

#endif /* SRC_LIBEXT_MEMORY_HPP_ */
