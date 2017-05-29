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

FileDescriptorCache::FileDescriptorCache()
{
	// TODO Auto-generated constructor stub

}

FileDescriptorCache::~FileDescriptorCache()
{
	// TODO Auto-generated destructor stub
}

FileDesc FileDescriptorCache::OpenAt(FileDesc& atdir,
		const std::string& relname, int mode)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	FileDescImpl fdi {atdir, -1, mode, relname};

	m_last_file_desc++;

	m_cache.insert(std::make_pair(m_last_file_desc, fdi));

	return m_last_file_desc;
}

int FileDescriptorCache::Lock(FileDesc& fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	// Check for an existing entry.  There should be one.
	auto fdit = m_cache.find(fd);

	// See if we have to create a new system file descriptor for it.
	if(fdit->second.m_system_descriptor < 0)
	{
		// We do.  See if we are out of system file descriptors, and need to close one.
		if(m_num_sys_fds_in_use == m_max_sys_fds)
		{
			FreeSysFileDesc();
		}

		int atdir = Lock(fdit->second.m_atdir_fd);
		fdit->second.m_system_descriptor = openat(atdir, fdit->second.m_atdir_relative_name.c_str(), fdit->second.mode);
		++m_num_sys_fds_in_use;
		Unlock(fdit->second.m_atdir_fd);
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
	Touch(fd);
	m_fd_fifo.pop_back();

	// Close the system file descriptor.
	auto fdiit = m_cache.find(fd);
	close(fdiit->second.m_system_descriptor);
	fdiit->second.m_system_descriptor = -1;
	--m_num_sys_fds_in_use;
}

void FileDescriptorCache::Close(FileDesc& fd)
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	// Unlock first if we need to.
	if(m_locked_fds.count(fd) == 1)
	{
		UnlockImpl(fd);
	}

	// Remove fd from the cache.
	m_cache.erase(fd);
}

void FileDescriptorCache::Touch(const FileDesc& fd)
{
	// Find the FileDesc.
	auto fdit = std::find_if(m_fd_fifo.begin(), m_fd_fifo.end(), [&fd](const FileDesc& it){ return it == fd; });

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
		else
		{
			// Found a descriptor in the cache which hasn't been used in a while.
			// close() it so we can re-use it.
			auto fdiit = m_cache.find(*fdit);
			close(fdiit->second.m_system_descriptor);
			fdiit->second.m_system_descriptor = -1;
			m_fd_fifo.erase(fdit);
			--m_num_sys_fds_in_use;
			return;
		}
	}

	// Never should get here unless we have too many locked files.
	throw FileException("Too many locked files.");
}
