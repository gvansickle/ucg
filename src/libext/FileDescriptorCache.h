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

#ifndef SRC_LIBEXT_FILEDESCRIPTORCACHE_H_
#define SRC_LIBEXT_FILEDESCRIPTORCACHE_H_

#include <config.h>

#include "FileDescriptor.hpp"
#include "filesystem.hpp"

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <string>
#include <mutex>


struct FileDesc
{
	uint32_t m_nonsys_descriptor {0};

	FileDesc() = default;
	explicit FileDesc(uint32_t fd) noexcept { m_nonsys_descriptor = fd; };
	FileDesc(const FileDesc& other) noexcept { m_nonsys_descriptor = other.m_nonsys_descriptor; };
	FileDesc(FileDesc&& other) noexcept { m_nonsys_descriptor = other.m_nonsys_descriptor; };
	FileDesc& operator=(const FileDesc& other) { m_nonsys_descriptor = other.m_nonsys_descriptor; return *this; };
	FileDesc& operator=(FileDesc&& other) { m_nonsys_descriptor = other.m_nonsys_descriptor; return *this; };
	~FileDesc() = default;

	bool operator==(const FileDesc& other) const noexcept { return m_nonsys_descriptor == other.m_nonsys_descriptor; };
	bool empty() const noexcept { return m_nonsys_descriptor == 0; };
};

struct FileDescImpl
{
	FileDesc m_atdir_fd;
	int m_system_descriptor = -1;
	int m_mode = 0;
	std::string m_atdir_relative_name;
};

namespace std
{
	/// Injects a specialization of std::hash<> into std:: for FileDesc.
	template <>
	struct hash<FileDesc>
	{
		std::size_t operator()(FileDesc fd) const
		{
			const std::size_t h1 (std::hash<decltype(fd.m_nonsys_descriptor)>{}(fd.m_nonsys_descriptor));
			return h1;
		}
	};

	/// Injects a specialization of std::hash<> into std:: for FileDescImpl.
	template <>
	struct hash<FileDescImpl>
	{
		std::size_t operator()(const FileDescImpl &p) const
		{
			const std::size_t h1 (std::hash<FileDesc>{}(p.m_atdir_fd));
			const std::size_t h2 (std::hash<std::string>{}(p.m_atdir_relative_name));
			return h1 ^ (h2 << 1);
		}
	};
}

/*
 *
 */
class FileDescriptorCache
{
public:
	FileDescriptorCache();
	~FileDescriptorCache();

	static FileDescriptorCache* Get() noexcept;

	FileDesc GetAT_FDCWD();

	bool DescriptorIsEmpty(const FileDesc& fd) const noexcept { return fd.empty(); };

	FileDesc OpenAt(const FileDesc& atdir, const std::string& relname, int mode);
	int Lock(FileDesc& fd);
	void Unlock(FileDesc& fd);
	FileDesc Dup(FileDesc fd);
	void Close(FileDesc& fd);

private:

	std::mutex m_mutex;
	uint32_t m_last_file_desc {0};
	uint32_t m_num_sys_fds_in_use {0};
	uint32_t m_max_sys_fds {100};
	std::unordered_map<FileDesc, FileDescImpl> m_cache;
	std::unordered_set<FileDesc> m_locked_fds;
	std::deque<FileDesc> m_fd_fifo;

	FileDesc OpenAtImpl(const FileDesc& atdir, const std::string& relname, int mode);
	int LockImpl(FileDesc& fd);
	void UnlockImpl(FileDesc& fd);
	void Touch(FileDesc& fd);
	void FreeSysFileDesc();
};

#endif /* SRC_LIBEXT_FILEDESCRIPTORCACHE_H_ */
