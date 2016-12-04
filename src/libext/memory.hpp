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
 * Everything anyone could ever hope for in a memory allocation interface.
 */
inline void * overaligned_alloc(std::size_t alignment, std::size_t min_bytes_needed) ATTR_ALLOC_SIZE(2)
{
	static long page_size = sysconf(_SC_PAGESIZE);

	// aligned_alloc() requires size == a power of 2 of the alignment.
	/// @todo Additionally, if the end of the requested size is within ??? bytes of the next page, we want to allocate the next page as well,
	/// so that we don't have to worry about reading past the end of the buffer causing a page fault.
	/// For now, we'll over-compensate and just allocate an additional page.
}


#endif /* SRC_LIBEXT_MEMORY_HPP_ */
