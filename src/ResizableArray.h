/*
 * Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <new>

template<typename T>
class ResizableArray
{
public:
	ResizableArray() = default;
	~ResizableArray() noexcept
	{
		if(m_current_buffer!=nullptr)
		{
			::operator delete(m_current_buffer);
		}
	};

	T * data() const noexcept { return m_current_buffer; };

	void reserve_no_copy(std::size_t needed_size)
	{
		if(m_current_buffer==nullptr || m_current_buffer_size < needed_size)
		{
			// Need to allocate a new raw buffer.
			if(m_current_buffer!=nullptr)
			{
				::operator delete(m_current_buffer);
			}

			m_current_buffer_size = needed_size;
			m_current_buffer = static_cast<T*>(::operator new(m_current_buffer_size));
		}
	}

private:

	std::size_t m_current_buffer_size { 0 };
	T *m_current_buffer { nullptr };
};

#endif /* SRC_RESIZABLEARRAY_H_ */
