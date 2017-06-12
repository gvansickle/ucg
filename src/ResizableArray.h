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

#ifndef SRC_RESIZABLEARRAY_H_
#define SRC_RESIZABLEARRAY_H_

#include <config.h>

#include <experimental/memory_resource>
#include <cstdlib>
#include <cstring> // For std::memset().

#include <libext/hints.hpp>
#include <libext/memory.hpp>

#include "libext/Logger.h"

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


	ResizableArray() noexcept = default;
	~ResizableArray() noexcept
	{
		if(m_current_buffer!=nullptr)
		{
			std::free(m_current_buffer);
		}
	};

	const_pointer data() const noexcept ATTR_MALLOC ATTR_PURE { return m_current_buffer; };

	void reserve_no_copy(std::size_t needed_size, std::size_t needed_alignment)  ATTR_ALLOC_SIZE(1)
	{
		if(m_current_buffer==nullptr || m_current_buffer_size < needed_size || m_current_buffer_alignment < needed_alignment)
		{
			// Need to allocate a new raw buffer.
			if(m_current_buffer!=nullptr)
			{
				std::free(m_current_buffer);
			}

			/// @note See overaligned_alloc() for requirements on params.
			m_current_buffer = static_cast<pointer>(overaligned_alloc(needed_alignment, needed_size));

			// We might have gotten a more-aligned block than we requested.
			m_current_buffer_alignment = 1U << count_trailing_zeros((uintptr_t)m_current_buffer);
			m_current_buffer_size = needed_size+1024/8;  /// @todo Hackish, relies on special knowledge of overaligned_alloc()'s internals.

			LOG(INFO) << "reserve_no_copy() realloc: needed_size=" << needed_size << ", needed_alignment=" << needed_alignment
					<< ", returned size=" << m_current_buffer_alignment << ", returned alignment =" << m_current_buffer_alignment;
		}

		// Zero-out the trailing vector's worth of extra space.
		std::memset(m_current_buffer+needed_size, 0, 1024/8);
	}

private:

	/// The number of bytes currently allocated.  This will be larger than the last requested size.
	std::size_t m_current_buffer_size { 0 };
	std::size_t m_current_buffer_alignment { 0 };
	pointer m_current_buffer { nullptr };
};

#endif /* SRC_RESIZABLEARRAY_H_ */
