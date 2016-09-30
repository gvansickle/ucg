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

#include <sys/stat.h>
#include <sys/types.h> // for dev_t, ino_t
// Don't know where the name "libgen" comes from, but this is where POSIX says dirname() and basename() are declared.
/// There are two basename()s.  GNU basename, from string.h, and POSIX basename() from libgen.h.
/// See notes here: https://linux.die.net/man/3/dirname
/// Of course they behave slightly differently: GNU version returns an empty string if the path has a trailing slash, and doesn't modify it's argument.
/// To complicate matters further, the glibc version of the POSIX function versions do modify their args.
/// And at least NetBSD behaves similarly.  So, include the POSIX versions and we'll try to clean this mess up below.
#include <libgen.h>

/// @note Because we included libgen.h above, we shouldn't get the GNU version from this #include of string.h.
#include <string.h>
#include <string>

#include "integer.hpp"

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

namespace portable
{

/**
 * A more usable and portable replacement for glibc and POSIX dirname().
 *
 * @param path  const ref to a path string.  Guaranteed to not be modified in any way by the function call.
 * @return  A std::string representing the path to return the directory of.  Guaranteed to be a normal std::string with which you may do
 *          whatever you can do with any other std::string.
 */
inline std::string dirname(const std::string &path)
{
	// Get a copy of the path string which dirname() can modify all it wants.
	char * modifiable_path = strdup(path.c_str());

	// Copy the output of dirname into a std:string.  We don't ever free the string dirname() returns
	// because it's either a static buffer, or it's a pointer to modifiable_path.  The latter we'll free below.
	std::string retval(::dirname(modifiable_path));

	free(modifiable_path);

	return retval;
}

}

#endif /* SRC_LIBEXT_FILESYSTEM_HPP_ */
