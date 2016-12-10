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

#if !defined(HAVE_OPENAT) || HAVE_OPENAT == 0
// No native openat() support.  Assume no *at() functions at all.  Stubs to allow this to compile for now.
#define AT_FDCWD -200
#define AT_NO_AUTOMOUNT 0
inline int openat(int at_fd, const char *fn, int flags) { return -1; };
inline DIR *fdopendir(int fd) { return nullptr; };
inline int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) { return -1; };
#endif

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
}

DirTree::~DirTree()
{
}


void DirTree::Read(std::vector<std::string> start_paths [[maybe_unused]])
{
#if 0 /// @obsolete
	///int num_entries {0};
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
#endif
}
