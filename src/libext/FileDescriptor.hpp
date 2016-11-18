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

/** @file FileDescriptor.hpp
 *
 */

#ifndef SRC_LIBEXT_FILEDESCRIPTOR_HPP_
#define SRC_LIBEXT_FILEDESCRIPTOR_HPP_

#include <future/shared_mutex.hpp>
#include "../Logger.h"

/**
 * Wrapper for C's 'int' file descriptor.
 * This class adds C++ RAII abilities and correct copy and move semantics to a POSIX file descriptor.
 */
class FileDescriptor
{
	/// @see the similar types in FileID.h for an explanation as to why these types exist even in C++11.
	using MutexType = std::shared_mutex;
	using ReaderLock = std::shared_lock<MutexType>;
	using WriterLock = std::unique_lock<MutexType>;

	/// Mutex for locking in copy and move constructors and some operations.
	mutable MutexType m_mutex;

public:
	/// Default constructor.
	/// @note Can't be noexcept, though only seems to break the compile on Cygwin gcc 5.4.0.
	FileDescriptor() { LOG(DEBUG) << "DEFAULT CONSTRUCTOR"; };

	explicit FileDescriptor(int fd) noexcept
	{
		WriterLock wl(m_mutex);
		m_file_descriptor = fd;
		LOG(DEBUG) << "Explicitly assigned file descriptor: " << m_file_descriptor;
	};

	/// Copy constructor will dup the other's file descriptor.
	FileDescriptor(const FileDescriptor &other) noexcept
	{
		ReaderLock rl(other.m_mutex);

		LOG(DEBUG) << "copy constructor called.";

		if(!other.unlocked_empty())
		{
			m_file_descriptor = dup(other.m_file_descriptor);
			LOG(DEBUG) << "duped fd=" << other.m_file_descriptor << " to fd=" << m_file_descriptor << ".";
		}
		else
		{
			// Other has an invalid fd, just copy the value.
			m_file_descriptor = other.m_file_descriptor;
			LOG(DEBUG) << "other was invalid, no dup. fd=" << m_file_descriptor;
		}
	}

	/// Move constructor.  Ownership of the fd will be transferred from other to this.
	/// For a move, the #other FileDescriptor has to be invalidated.  Otherwise,
	/// when it is destroyed, it will close the file, which it no longer owns.
	FileDescriptor(FileDescriptor&& other) noexcept
	{
		WriterLock wl(other.m_mutex);
		LOG(DEBUG) << "move constructor called.";

		m_file_descriptor = other.m_file_descriptor;
		LOG(DEBUG) << "moved file descriptor: " << m_file_descriptor;

		if(!other.unlocked_empty())
		{
			other.m_file_descriptor = cm_invalid_file_descriptor;
		}
	}

	/// Destructor.  Closes #m_file_descriptor if it's valid.
	~FileDescriptor() noexcept
	{
		// @note No locking here.  If anyone was trying to read or write us, they'd have
		// to have (possibly shared) ownership (right?), and hence we wouldn't be getting destroyed.
		WriterLock wl(m_mutex);
		LOG(DEBUG) << "DESTRUCTOR, have file descriptor: " << m_file_descriptor;
		if(!unlocked_empty())
		{
			LOG(DEBUG) << "closing file descriptor: " << m_file_descriptor;
			close(m_file_descriptor);
			m_file_descriptor = cm_invalid_file_descriptor;
		}
	};

	/// The default copy-assignment operator won't do the right thing.
	FileDescriptor& operator=(const FileDescriptor& other) noexcept
	{
		if(this != &other)
		{
			WriterLock this_lock(m_mutex, std::defer_lock);
			ReaderLock other_lock(other.m_mutex, std::defer_lock);
			std::lock(this_lock, other_lock);

			LOG(DEBUG) << "copy assignment called.";

			if(!unlocked_empty())
			{
				close(m_file_descriptor);
				LOG(DEBUG) << "closing file descriptor: " << m_file_descriptor;
			}

			if(other.unlocked_empty())
			{
				// Other fd isn't valid, just copy it.
				m_file_descriptor = other.m_file_descriptor;
				LOG(DEBUG) << "copied invalid file descriptor: " << m_file_descriptor;
			}
			else
			{
				m_file_descriptor = dup(other.m_file_descriptor);
				if(m_file_descriptor < 0)
				{
					ERROR() << "dup() failure: " << LOG_STRERROR();
				}
				LOG(DEBUG) << "duped file descriptor: " << m_file_descriptor;
			}
		}
		return *this;
	}

	/// The default move-assignment operator won't do the right thing.
	FileDescriptor& operator=(FileDescriptor&& other) noexcept
	{
		if(this != &other)
		{
			WriterLock this_lock(m_mutex, std::defer_lock);
			WriterLock other_lock(other.m_mutex, std::defer_lock);
			std::lock(this_lock, other_lock);

			LOG(DEBUG) << "move assignment called.";

			// Step 1: Release any resources this owns.
			if(!unlocked_empty())
			{
				close(m_file_descriptor);
				LOG(DEBUG) << "closing file descriptor: " << m_file_descriptor;
			}

			// Step 2: Take other's resources.
			m_file_descriptor = other.m_file_descriptor;
			LOG(DEBUG) << "moved file descriptor: " << m_file_descriptor;

			// Step 3: Set other to a destructible state.
			// In particular here, this means invalidating its file descriptor,
			// so it isn't closed when other is deleted.
			other.m_file_descriptor = cm_invalid_file_descriptor;
		}

		// Step 4: Return *this.
		return *this;
	}

	/// Allow read access to the underlying int.
	int GetFD() const noexcept
	{
		ReaderLock rl(m_mutex);
		return m_file_descriptor;
	};

	int GetDupFD() const noexcept
	{
		ReaderLock rl(m_mutex);
		int retval = dup(m_file_descriptor);
		return retval;
	}

	/// Returns true if this FileDescriptor isn't a valid file descriptor.
	inline bool empty() const noexcept
	{
		ReaderLock rl(m_mutex);
		return unlocked_empty();
	}

private:
	static constexpr int cm_invalid_file_descriptor = -987;

	mutable int m_file_descriptor { cm_invalid_file_descriptor };

	inline bool unlocked_empty() const noexcept { return m_file_descriptor < 0; };
};


inline FileDescriptor make_shared_fd(int fd)
{
	FileDescriptor retval(fd);
	/// Should get returned by RVO, or move.
	return retval;
}

static_assert(std::is_copy_constructible<FileDescriptor>::value, "FileDescriptor must be copy constructible.");
static_assert(std::is_move_constructible<FileDescriptor>::value, "FileDescriptor must be move constructible.");
static_assert(std::is_assignable<FileDescriptor, FileDescriptor>::value, "FileDescriptor must be assignable to itself.");
static_assert(std::is_copy_assignable<FileDescriptor>::value, "FileDescriptor must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileDescriptor>::value, "FileDescriptor must be move assignable to itself.");

#endif /* SRC_LIBEXT_FILEDESCRIPTOR_HPP_ */
