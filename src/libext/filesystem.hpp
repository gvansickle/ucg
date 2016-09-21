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

#ifndef SRC_LIBEXT_FILESYSTEM_HPP_
#define SRC_LIBEXT_FILESYSTEM_HPP_

#include <config.h>

#include <fcntl.h> // For openat() etc.
#include <sys/stat.h>
#include <sys/types.h> // for dev_t, ino_t
#include <dirent.h>

#include "integer.hpp"

/// @name Take care of some portability issues.
/// OSX (clang-600.0.54) (based on LLVM 3.5svn)/x86_64-apple-darwin13.4.0:
/// - No AT_FDCWD, no openat, no fdopendir, no fstatat.
/// Cygwin:
/// - No O_NOATIME, no AT_NO_AUTOMOUNT.
/// Linux:
/// - No O_SEARCH.
/// @{
#ifndef O_SEARCH
// O_SEARCH is POSIX.1-2008, but not defined on at least Linux/glibc 2.24.
// Possible reason, quoted from the standard: "Since O_RDONLY has historically had the value zero, implementations are not able to distinguish
// between O_SEARCH and O_SEARCH | O_RDONLY, and similarly for O_EXEC."
#define O_SEARCH 0
#endif
/// @}


using dev_ino_pair_type = uint_t<(sizeof(dev_t)+sizeof(ino_t))*8>::fast;

struct dev_ino_pair
{
	dev_ino_pair() = default;
	dev_ino_pair(dev_t d, ino_t i) noexcept { m_val = d, m_val <<= sizeof(ino_t)*8, m_val |= i; };

	dev_ino_pair_type m_val { 0 };
};


/**
 * Checks two file descriptors (file, dir, whatever) and checks if they are referring to the same entity.
 *
 * @param fd1
 * @param fd2
 * @return true if fd1 and fd2 are fstat()able and refer to the same entity, false otherwise.
 */
inline bool is_same_file(int fd1, int fd2)
{
	struct stat s1, s2;

	if(fstat(fd1, &s1) < 0)
	{
		return false;
	}
	if(fstat(fd2, &s2) < 0)
	{
		return false;
	}

	if(
		(s1.st_dev == s2.st_dev) // Same device
		&& (s1.st_ino == s2.st_ino) // Same inode
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Examines the given #path and determines if it is absolute.
 *
 * @param path
 * @return
 */
inline bool is_pathname_absolute(const std::string &path) noexcept
{
#if 1 // == IS_POSIX
	if(path[0] == '/')
	{
		return true;
	}
	else
	{
		return false;
	}
#else /// @todo Handle Windows etc.
#error "Only POSIX-like systems currently supported."
	return false;
#endif
}


inline DIR* opendirat(int at_dir, const char *name)
{
	constexpr int openat_dir_search_flags = O_SEARCH ? O_SEARCH : O_RDONLY;

	int file_fd = openat(at_dir, name, openat_dir_search_flags | O_DIRECTORY | O_NOCTTY);
	if(file_fd < 0)
	{
		perror("openat() failed");
	}
	DIR* d = fdopendir(file_fd);

	return d;
}

#endif /* SRC_LIBEXT_FILESYSTEM_HPP_ */
