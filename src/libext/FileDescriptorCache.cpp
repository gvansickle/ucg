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

#include <config.h>

#include "FileDescriptorCache.h"

#include "filesystem.hpp"

#include <algorithm>

static FileDescriptorCache FileDescriptorCacheSingleton;

FileDescriptorCache* FileDescriptorCache::Get() noexcept
{
	return &FileDescriptorCacheSingleton;
}

#if 0
FileDesc::FileDesc(const FileDesc& other)
{
	*this = FileDescriptorCache::Get()->Dup(other);
};
#endif


FileDescriptorCache::FileDescriptorCache()
{
	// Create the AT_FDCWD.
	FileDescImpl fdi { FileDesc(0), -1, O_SEARCH | O_DIRECTORY | O_NOCTTY, "."};
	++m_last_file_desc;
	FileDesc fd {m_last_file_desc};
	//m_cache.insert(std::move(std::make_pair(std::move(fd), std::move(fdi))));
	m_cache.emplace(fd, std::move(fdi));
	m_fd_fifo.push_back(fd);

	// Keep AT_FDCWD locked for the duration of the program.
	Lock(fd);
}

FileDescriptorCache::~FileDescriptorCache()
{
	///Unlock(GetAT_FDCWD());
}

FileDesc FileDescriptorCache::GetAT_FDCWD()
{
	FileDesc fd{1};
	return fd;
}

FileDesc FileDescriptorCache::OpenAt(const FileDesc& atdir, const std::string& relname, int mode)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	return OpenAtImpl(atdir, relname, mode);
}

FileDesc FileDescriptorCache::OpenAtImpl(const FileDesc& atdir, const std::string& relname, int mode)
{
	FileDescImpl fdi {atdir, -1, mode, relname};

	++m_last_file_desc;
	FileDesc fd {m_last_file_desc};
	m_cache.emplace(fd, std::move(fdi));
	m_fd_fifo.push_back(fd);

	return fd;
}

int FileDescriptorCache::Lock(FileDesc& fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	return LockImpl(fd);
}

int FileDescriptorCache::LockImpl(FileDesc& fd)
{
	// Check for an existing entry.  There should be one.
	auto fdit = m_cache.find(fd);

	if(fdit == m_cache.end())
	{
		throw FileException("File lock failure");
	}

	// See if we have to create a new system file descriptor for it.
	if(fdit->second.m_system_descriptor < 0)
	{
		// We do.  See if we are out of system file descriptors, and need to close one.
		if(m_num_sys_fds_in_use == m_max_sys_fds)
		{
			FreeSysFileDesc();
		}

		if(fdit->second.m_atdir_fd.m_nonsys_descriptor > 0)
		{
			int atdir = LockImpl(fdit->second.m_atdir_fd);
			// The LockImpl() may have used the fd we just freed.  See if we are out of system file descriptors, and need to close one.
			if(m_num_sys_fds_in_use == m_max_sys_fds)
			{
				FreeSysFileDesc();
			}
			fdit->second.m_system_descriptor = openat(atdir, fdit->second.m_atdir_relative_name.c_str(), fdit->second.m_mode);
			UnlockImpl(fdit->second.m_atdir_fd);
		}
		else
		{
			// We've recursed back to the AT_FDCWD.  Stop recursing.
			fdit->second.m_system_descriptor = open(fdit->second.m_atdir_relative_name.c_str(), fdit->second.m_mode);
		}
		++m_num_sys_fds_in_use;
	}
	// else we already have a system file descriptor, nothing to do.

	// Touch this FileDesc to move it to the front of the FIFO.
	Touch(fd);

	// Move the FileDesc to the locked hash so we don't delete it.
	m_locked_fds.insert(fd);

	// Return the system file descriptor.
	return fdit->second.m_system_descriptor;
}

void FileDescriptorCache::Unlock(FileDesc& fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	UnlockImpl(fd);
}

void FileDescriptorCache::UnlockImpl(FileDesc& fd)
{
	// Remove fd from the lock hash.
	m_locked_fds.erase(fd);

	// Remove from the LRU FIFO.
	//Touch(fd);
	//m_fd_fifo.pop_back();
}

FileDesc FileDescriptorCache::Dup(FileDesc fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	auto fdit = m_cache.find(fd);
	return OpenAtImpl(fdit->second.m_atdir_fd, fdit->second.m_atdir_relative_name, fdit->second.m_mode);
}

void FileDescriptorCache::Close(FileDesc& fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	// Unlock first if we need to.
	if(m_locked_fds.count(fd) == 1)
	{
		UnlockImpl(fd);
	}

	// Close the system file descriptor.
	auto fdiit = m_cache.find(fd);
	if(fdiit == m_cache.end())
	{
		throw FileException("INTERNAL ERROR: file being closed not in cache");
	}
	else if(fdiit->second.m_system_descriptor < 0)
	{
		throw FileException("INTERNAL ERROR: file being closed has no descriptor");
	}
	close(fdiit->second.m_system_descriptor);
	fdiit->second.m_system_descriptor = -1;
	--m_num_sys_fds_in_use;

	// Remove fd from the cache.
	//m_cache.erase(fd);
}

void FileDescriptorCache::Touch(FileDesc& fd)
{
	// Find the FileDesc.
	auto fdit = std::find(m_fd_fifo.begin(), m_fd_fifo.end(), fd);

	if(fdit == m_fd_fifo.end())
	{
		throw FileException("INTERNAL ERROR: fd not in fifo");
	}

	// Move it to the end of the deque.
	m_fd_fifo.erase(fdit);
	m_fd_fifo.push_back(fd);
}

void FileDescriptorCache::FreeSysFileDesc()
{
	for(auto fdit = m_fd_fifo.begin(); fdit < m_fd_fifo.end(); ++fdit)
	{
		if(m_locked_fds.count(*fdit) == 1)
		{
			// This fd is locked, skip to the next one.
			continue;
		}

		// File isn't locked, does it have an allocated system file descriptor?
		auto fdiit = m_cache.find(*fdit);

		if(fdiit == m_cache.end())
		{
			throw FileException("INTERNAL ERROR: error while freeing sys fd");
		}

		if(fdiit->second.m_system_descriptor < 0)
		{
			// No, skip it.
			continue;
		}
		else
		{
			// Found a descriptor in the cache which hasn't been used in a while.
			// close() it so we can re-use it.
			close(fdiit->second.m_system_descriptor);
			fdiit->second.m_system_descriptor = -1;
			//m_fd_fifo.erase(fdit);
			--m_num_sys_fds_in_use;
			return;
		}
	}

	// Never should get here unless we have too many locked files.
	throw FileException("Too many locked files.");
}
