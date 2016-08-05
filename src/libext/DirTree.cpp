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

/** @file  */

#include <config.h>

#include "DirTree.h"

#include <fcntl.h> // For AT_FDCWD, AT_NO_AUTOMOUNT
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>

#include <stdexcept>
#include <iostream>
#include <queue>
#include <map>
#include <memory>
#include <algorithm>

#define EXPAND_MACRO_HELPER(x) #x
#define EXPAND_MACRO(p) EXPAND_MACRO_HELPER(p)
#define STATIC_MESSAGE_HELPER(m) _Pragma(#m)
#define STATIC_MESSAGE(m) STATIC_MESSAGE_HELPER(message #m)

/// @name Take care of some portability issues.
/// OSX (clang-600.0.54) (based on LLVM 3.5svn)/x86_64-apple-darwin13.4.0:
/// - No AT_FDCWD, no openat, no fdopendir, no fstatat.
/// Cygwin:
/// - No O_NOATIME, no AT_NO_AUTOMOUNT.
/// Linux:
/// - No O_SEARCH.
/// @{
#ifndef AT_NO_AUTOMOUNT
#define AT_NO_AUTOMOUNT 0  // Not defined on at least Cygwin.
#endif
#ifndef O_NOATIME
// From "The GNU C Library" manual <https://www.gnu.org/software/libc/manual/html_mono/libc.html#toc-File-System-Interface-1>
// 13.14.3 I/O Operating Modes: "Only the owner of the file or the superuser may use this bit."
#define O_NOATIME 0  // Not defined on at least Cygwin.
#endif
#ifndef O_SEARCH
// O_SEARCH is POSIX.1-2008, but not defined on at least Linux/glibc 2.24.
// Possible reason, quoted from the standard: "Since O_RDONLY has historically had the value zero, implementations are not able to distinguish
// between O_SEARCH and O_SEARCH | O_RDONLY, and similarly for O_EXEC."
#define O_SEARCH 0
#endif

#ifdef _POSIX_OPEN_MAX
// Posix specifies the minimum of this to be 20.
//STATIC_MESSAGE(Posix open max is _POSIX_OPEN_MAX)
//STATIC_MESSAGE("_POSIX_OPEN_MAX=" EXPAND_MACRO(_POSIX_OPEN_MAX))
//#pragma message "_POSIX_OPEN_MAX=" EXPAND_MACRO(_POSIX_OPEN_MAX)
#endif

///@}

DirTree::DirTree()
{
	// TODO Auto-generated constructor stub

}

DirTree::~DirTree()
{
	// TODO Auto-generated destructor stub
}

class AtFD
{
public:
	AtFD() = default;
	AtFD(const AtFD& other)
	{
		inc_refcount(other.m_real_file_descriptor);
		m_real_file_descriptor = other.m_real_file_descriptor;
	}

	~AtFD()
	{
		if(m_real_file_descriptor == -1 || m_real_file_descriptor == AT_FDCWD || m_real_file_descriptor < 0)
			return;

		m_refcount_map[m_real_file_descriptor]--;
		if(m_refcount_map[m_real_file_descriptor] == 0)
		{
			//std::cout << "REF COUNT == 0, DELETE AND CLOSE: " << m_real_file_descriptor << '\n';
			//m_refcount_map.erase(it);
			close(m_real_file_descriptor);
		}
	}

	int get_at_fd() const { return m_real_file_descriptor; };

	static AtFD make_shared_dupfd(int fd)
	{
		AtFD retval;

		if(fd == AT_FDCWD)
		{
			retval.m_real_file_descriptor = fd;
			return retval;
		}
		else if(fd < 0)
		{
			throw std::out_of_range("fd < 0");
		}

		//auto it = m_refcount_map.find(fd);
		if(0 == get_refcount_or_allocate(fd))
		{
			// Hasn't been duped yet.
			retval.m_real_file_descriptor = dup(fd);
			//std::cout << "NEW dup, original = " << fd << ", new = " << retval.m_real_file_descriptor << '\n';
			auto cur_refcnt = get_refcount_or_allocate(retval.m_real_file_descriptor);
			if(cur_refcnt != 0) { throw std::out_of_range("new fd's refcount not 0"); };
			inc_refcount(retval.m_real_file_descriptor);
		}
		else
		{
			throw std::out_of_range("original fd is in map");
		}

		return retval;
	}

	void operator=(AtFD other)
	{
		/// @todo get this to work here: std::swap(*this, other);
		inc_refcount(other.m_real_file_descriptor);
		m_real_file_descriptor = other.m_real_file_descriptor;
	};

	bool is_valid() const noexcept
	{
		if (m_real_file_descriptor > 0 || m_real_file_descriptor == AT_FDCWD)
		{
			return true;
		}
		else
		{
			return false;
		}
	};

private:
	//static std::map<int, size_t> m_refcount_map;
	static std::vector<size_t> m_refcount_map;
	static size_t get_refcount_or_allocate(int fd)
	{
		if(fd < 0)
		{
			return 0;
		}

		long unsigned int ufd = fd;

		if(ufd >= m_refcount_map.size())
		{
			// Off the end, reallocate the vector.
			decltype(m_refcount_map)::size_type new_size = std::max(ufd+1, m_refcount_map.size()*2);
			m_refcount_map.resize(new_size);
		}
		return m_refcount_map[fd];
	}
	static void inc_refcount(int fd)
	{
		if(fd >= 0)
		{
			m_refcount_map[fd]++;
		};
	};
	static bool dec_refcount(int fd)
	{
		if(fd >= 0)
		{
			m_refcount_map[fd]--;
			if(m_refcount_map[fd] == 0)
			{
				// It's zero, tell caller to close the fd.
				return true;
			}
		}
		return false;
	}

	int m_real_file_descriptor {-1};
};

std::vector<size_t> AtFD::m_refcount_map(256);

//std::map<int, size_t> AtFD::m_refcount_map;


class DirStackEntry
{
public:
	DirStackEntry(std::shared_ptr<DirStackEntry> parent_dse, AtFD parent_dir, std::string path)
	{
		m_parent_dir = parent_dse;
		m_at_fd = parent_dir;
		m_path = path;
	};

	DirStackEntry(const DirStackEntry& other) = default;

	~DirStackEntry() = default;

	std::string get_name()
	{
		if(m_lazy_full_name.empty())
		{
			std::string retval;

			if(m_parent_dir)
			{
				retval = m_parent_dir->get_name() + "/";
			}
			retval += m_path;

			m_lazy_full_name.assign(retval);

			return retval;
		}
		else
		{
			return m_lazy_full_name;
		}
	}

	int get_at_fd() const noexcept { return m_at_fd.get_at_fd(); };

	std::string m_path;
	AtFD m_at_fd;

private:
	std::string m_lazy_full_name {};
	std::shared_ptr<DirStackEntry> m_parent_dir { nullptr };
};


void DirTree::Read(std::vector<std::string> start_paths)
{
	int num_entries {0};
	struct stat statbuf;
	DIR *d {nullptr};
	struct dirent *dp {nullptr};
	int file_fd;

	std::queue<std::shared_ptr<DirStackEntry>> dir_stack;
	for(auto p : start_paths)
	{
		// AT_FDCWD == Start at the cwd of the process.
		dir_stack.push(std::make_shared<DirStackEntry>(DirStackEntry(nullptr, AtFD::make_shared_dupfd(AT_FDCWD), p)));

		/// @todo The start_paths can be files or dirs.  Currently the loop below will only work if they're dirs.
	}

	while(!dir_stack.empty())
	{
		auto dse = dir_stack.front();
		dir_stack.pop();

		int openat_dir_search_flags = O_SEARCH ? O_SEARCH : O_RDONLY;
		file_fd = openat(dse->get_at_fd(), dse->m_path.c_str(), openat_dir_search_flags | O_DIRECTORY | O_NOATIME | O_NOCTTY);
		d = fdopendir(file_fd);
		if(d == nullptr)
		{
			// At a minimum, this wasn't a directory.
			perror("fdopendir");
		}

		// This will be the "at" directory for all files and directories found in this directory.
		AtFD next_at_dir;
		std::shared_ptr<DirStackEntry> dir_atfd;

		while ((dp = readdir(d)) != NULL)
		{
			bool is_dir {false};
			bool is_file {false};
			bool is_unknown {true};

#ifdef _DIRENT_HAVE_D_TYPE
			// Reject anything that isn't a directory or a regular file.
			// If it's DT_UNKNOWN, we'll have to do a stat to find out.
			is_dir = (dp->d_type == DT_DIR);
			is_file = (dp->d_type == DT_REG);
			is_unknown = (dp->d_type == DT_UNKNOWN);
			if(!is_file && !is_dir && !is_unknown)
			{
				// It's a type we don't care about.
				continue;
			}
#endif
			const char *dname = dp->d_name;
			if(dname[0] == '.' && (dname[1] == 0 || (dname[1] == '.' && dname[2] == 0)))
			{
				//std::cerr << "skipping: " << dname << '\n';
				continue;
			}

			if(is_unknown)
			{
				// Stat the filename using the at-descriptor.
				fstatat(file_fd, dname, &statbuf, AT_NO_AUTOMOUNT);
				is_dir = S_ISDIR(statbuf.st_mode);
				is_file = S_ISREG(statbuf.st_mode);
				if(is_dir || is_file)
				{
					is_unknown = false;
				}
			}

			if(is_unknown)
			{
				std::cerr << "cannot determine file type: " << dname << ", " << statbuf.st_mode << '\n';
				continue;
			}

			if(is_file)
			{
				std::cout << "File: " << '\n';//dse.get()->get_name() + "/" + dname << '\n';
			}
			else if(is_dir)
			{
				std::cout << "Dir: " << '\n';//dse.get()->get_name() + "/" + dname << '\n';

				if(!next_at_dir.is_valid())
				{
					next_at_dir = AtFD::make_shared_dupfd(file_fd);
				}

				dir_atfd = std::make_shared<DirStackEntry>(dse, next_at_dir, dname);

				dir_stack.push(dir_atfd);
			}
		}

		closedir(d);
	}
}
