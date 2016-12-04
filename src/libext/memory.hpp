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


#endif /* SRC_LIBEXT_MEMORY_HPP_ */
