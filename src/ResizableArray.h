/*
 * ResizeableArray.h
 *
 *  Created on: May 8, 2016
 *      Author: gary
 */

#ifndef SRC_RESIZABLEARRAY_H_
#define SRC_RESIZABLEARRAY_H_

#include <new>

template<typename T>
class ResizableArray
{
public:
	ResizableArray() = default;
	~ResizableArray()
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
