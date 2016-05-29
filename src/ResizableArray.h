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
	// Would pass this in as a defaulted template param, but gcc complains that "error: requested alignment is not an integer constant"
	// no matter what I do (including initializing this var to the template param).
	// Also can't make it a "static constexpr int alignment {32}", since that's broken on gcc ~4.8.4.
#define alignment 64

public:

	using element_type alignas(alignment) = T;

	using pointer = element_type *;

	using const_pointer = const element_type *;

//static_assert(alignof(decltype(*std::declval<const_pointer>())) != alignment, "alignment isn't working");_attribute__((aligned(alignment)))

	ResizableArray() noexcept = default;
	~ResizableArray() noexcept
	{
		if(m_current_buffer!=nullptr)
		{
			std::free(m_current_buffer);
		}
	};

	const_pointer data() const noexcept __attribute__((malloc)) { return m_current_buffer; };

	void reserve_no_copy(std::size_t needed_size)
	{
		if(m_current_buffer==nullptr || m_current_buffer_size < needed_size)
		{
			// Need to allocate a new raw buffer.
			if(m_current_buffer!=nullptr)
			{
				std::free(m_current_buffer);
			}

			m_current_buffer_size = needed_size;
			m_current_buffer = static_cast<pointer>(aligned_alloc(alignment, ((m_current_buffer_size*sizeof(element_type)+(alignment-1))/alignment)*alignment));
			//LOG(INFO) << "filedata alignof: " << alignof(*m_current_buffer) << "," << alignof(element_type) << std::endl;
		}
	}

private:
	std::size_t m_current_buffer_size { 0 };
	pointer m_current_buffer { nullptr };
#undef alignment
};

#endif /* SRC_RESIZABLEARRAY_H_ */
