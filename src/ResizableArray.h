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

#ifndef SRC_RESIZABLEARRAY_H_
#define SRC_RESIZABLEARRAY_H_

#include "config.h"

#include <libext/hints.hpp>

#include <cstdlib>
#ifdef HAVE_ALIGNED_ALLOC
// Nothing to do.
#elif HAVE_POSIX_MEMALIGN
// Create a thin wrapper around posix_memalign().
inline void* aligned_alloc(size_t algn, size_t size) { void *p=0; posix_memalign(&p, algn, size); return p; };
#else
#error "Could not find aligned memory allocator."
#endif

#include "Logger.h"
#include "libext/integer.hpp"

/**
 * This is sort of a poor-man's std::allocator<>, without the std.  We use it in the File() constructor
 * to get an uninitialized buffer to read the file data into.  By instantiating one of these objects prior to a loop of
 * File() constructions, we will simply recycle the same buffer unless we need a larger one, instead of
 * deleting/newing a brand-new buffer for every file we read in.  This can reduce allocation traffic considerably.
 * See FileScanner::Run() for this sort of usage.
 */
template<typename T>
class ResizableArray
{
public:

	using element_type = T;

	using pointer = element_type *;

	using const_pointer = const element_type *;

	/// @todo This static assert just doesn't work under clang 3.5.
	//static_assert(alignof(decltype(*std::declval<const_pointer>())) == alignment, "alignment isn't working"); //__attribute__((aligned(alignment)))

	ResizableArray() noexcept = default;
	~ResizableArray() noexcept
	{
		if(m_current_buffer!=nullptr)
		{
			std::free(m_current_buffer);
		}
	};

	const_pointer data() const noexcept ATTR_MALLOC { return m_current_buffer; };

	void reserve_no_copy(std::size_t needed_size, std::size_t needed_alignment)  ATTR_ALLOC_SIZE(1)
	{
		if(m_current_buffer==nullptr || m_current_buffer_size < needed_size || m_current_buffer_alignment < needed_alignment)
		{
			// Check that requested alignment is a power of two.
			if(!is_power_of_2(needed_alignment))
			{
				throw std::invalid_argument("requested alignment is not a power of 2");
			}

			// Need to allocate a new raw buffer.
			if(m_current_buffer!=nullptr)
			{
				std::free(m_current_buffer);
			}

			// aligned_alloc() requires size == a power of 2 of the alignment.
			/// @todo Additionally, if the end of the requested size is within ??? bytes of the next page, we want to allocate the next page as well,
			/// so that we don't have to worry about reading past the end of the buffer causing a page fault.
			auto requested_size = (needed_size + needed_alignment) - (needed_size & (needed_alignment-1));
			m_current_buffer = static_cast<pointer>(aligned_alloc(needed_alignment, requested_size));
			// We might have gotten a more-aligned block than we requested.
			m_current_buffer_alignment = 1U << (__builtin_clzll((uintptr_t)m_current_buffer));
			m_current_buffer_size = requested_size;

			LOG(INFO) << "reserve_no_copy() realloc: needed_size=" << needed_size << ", needed_alignment=" << needed_alignment
					<< ", requested_size=" << requested_size << ", requested_alignment= " << needed_alignment
					<< ", returned alignment =" << m_current_buffer_alignment;
		}
	}

private:

	/// The number of bytes currently allocated.  This will be larger than the last requested size.
	std::size_t m_current_buffer_size { 0 };
	std::size_t m_current_buffer_alignment { 0 };
	pointer m_current_buffer { nullptr };
};

#endif /* SRC_RESIZABLEARRAY_H_ */
