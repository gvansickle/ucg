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

/** @file filesystem.hpp
 * Take care of some filesystem API portability and convenience issues.
 */

#ifndef SRC_LIBEXT_FILESYSTEM_HPP_
#define SRC_LIBEXT_FILESYSTEM_HPP_

#include <config.h>

#include <cassert>
#include <cstdio>  // For perror() on FreeBSD.

#ifdef HAVE_PWD_H
#include <pwd.h> // for get_home_dir_name()-->getpwuid().
#endif

#include <fcntl.h> // For openat() and other *at() functions, AT_* defines.
#include <unistd.h> // For close().
#include <sys/stat.h>
#include <sys/types.h> // for dev_t, ino_t
// Don't know where the name "libgen" comes from, but this is where POSIX says dirname() and basename() are declared.
/// There are two basename()s.  GNU basename, from string.h, and POSIX basename() from libgen.h.
/// See notes here: https://linux.die.net/man/3/dirname
/// Of course they behave slightly differently: GNU version returns an empty string if the path has a trailing slash, and doesn't modify it's argument.
/// To complicate matters further, the glibc version of the POSIX function versions do modify their args.
/// And at least NetBSD behaves similarly.  So, include the POSIX versions and we'll try to clean this mess up below.
#include <libgen.h>
#include <dirent.h>

/// @note Because we included libgen.h above, we shouldn't get the GNU version from this #include of string.h.
#include <string.h>
#include <cstdlib>   // For free().
#include <string>
#include <iterator>   // For std::distance().
#include <future/type_traits.hpp>
#include <future/memory.hpp>
#include <future/shared_mutex.hpp>

#include "integer.hpp"
#include "Logger.h"


/// OSX (clang-600.0.54) (based on LLVM 3.5svn)/x86_64-apple-darwin13.4.0:
/// - No AT_FDCWD, no openat, no fdopendir, no fstatat.
/// Cygwin:
/// - No O_NOATIME, no AT_NO_AUTOMOUNT.
/// Linux:
/// - No O_SEARCH.
/// @{
#if !defined(HAVE_OPENAT) || HAVE_OPENAT == 0
// No native openat() support.  Assume no *at() functions at all.  Stubs to allow this to compile for now.
#define AT_FDCWD -200
#define AT_NO_AUTOMOUNT 0
inline int openat(int at_fd, const char *fn, int flags) { return -1; };
inline DIR *fdopendir(int fd) { return nullptr; };
inline int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) { return -1; };
#endif

// Cygwin has *at() functions, but is missing some AT_* defines.
#if !defined(AT_NO_AUTOMOUNT)
#define AT_NO_AUTOMOUNT 0
#endif

#if !defined(O_NOATIME)
// From "The GNU C Library" manual <https://www.gnu.org/software/libc/manual/html_mono/libc.html#toc-File-System-Interface-1>
// 13.14.3 I/O Operating Modes: "Only the owner of the file or the superuser may use this bit."
#define O_NOATIME 0  // Not defined on at least Cygwin.
#endif

#if !defined(O_SEARCH)
// O_SEARCH is POSIX.1-2008, but not defined on at least Linux/glibc 2.24.
// Possible reason, quoted from the standard: "Since O_RDONLY has historically had the value zero, implementations are not able to distinguish
// between O_SEARCH and O_SEARCH | O_RDONLY, and similarly for O_EXEC."
//
// What O_SEARCH does, from POSIX.1-2008 <http://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html>:
// "The openat() function shall be equivalent to the open() function except in the case where path specifies a relative path. In this case
// the file to be opened is determined relative to the directory associated with the file descriptor fd instead of the current working
// directory. If the access mode of the open file description associated with the file descriptor is not O_SEARCH, the function shall check
// whether directory searches are permitted using the current permissions of the directory underlying the file descriptor. If the access
// mode is O_SEARCH, the function shall not perform the check."
// So, it benefits us to specify O_SEARCH when we can.
#define O_SEARCH O_RDONLY
#endif

// Per POSIX.1-2008:
// "If path resolves to a non-directory file, fail and set errno to [ENOTDIR]."
#if !defined(O_DIRECTORY)
#define O_DIRECTORY 0
#endif

// Per POSIX.1-2008:
// "If set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal
// for the process. If path does not identify a terminal device, O_NOCTTY shall be ignored."
#if !defined(O_NOCTTY)
#define O_NOCTTY 0
#endif

// Non-POSIX Linux extension:
// "Obtain a file descriptor that can be used for two purposes: to indicate a location in the filesystem tree and to perform
// operations that act purely at the file descriptor level. [...]  [T]he file descriptor [can be passed] as the dirfd argument of
// openat(2) and the other "*at()" system calls."
#if !defined(O_PATH)
#define O_PATH 0
#endif
/// @}



/**
 * Class to throw for failures of file-related fuctions such as open()/fstat()/etc.
 */
struct FileException : public std::system_error
{
	FileException(const std::string &message, int errval = errno) : std::system_error(errval, std::generic_category(), message) {};
};
inline std::ostream& operator<<(std::ostream &out, const FileException &fe) noexcept
{
	return out << fe.what() << ": " << fe.code() << " - " << fe.code().message();
}



/**
 * Class intended to abstract the concept of a UUID for a file or directory.
 * @todo Currently only supports POSIX-like OSes, and not necessarily all filesystems.  Needs to be expanded.
 */
struct dev_ino_pair
{
	dev_ino_pair() = default;
	constexpr dev_ino_pair(dev_t d, ino_t i) noexcept : m_dev(d), m_ino(i) { };
	~dev_ino_pair() = default;

	inline bool operator<(const dev_ino_pair& other) const noexcept { return m_dev < other.m_dev || m_ino < other.m_ino; };

	inline bool operator==(dev_ino_pair other) const noexcept { return m_dev == other.m_dev && m_ino == other.m_ino; };

	inline bool empty() const noexcept { return m_dev == 0 && m_ino == 0; };

private:
	friend struct std::hash<dev_ino_pair>;

	dev_t m_dev {0};
	ino_t m_ino {0};
};

namespace std
{
	/// Injects a specialization of std::hash<> into std:: for dev_ino_pair.
	template <>
	struct hash<dev_ino_pair>
	{
		std::size_t operator()(const dev_ino_pair p) const
		{
			const std::size_t h1 (std::hash<dev_t>{}(p.m_dev));
			const std::size_t h2 (std::hash<dev_t>{}(p.m_ino));
			return h1 ^ (h2 << 1);
		}
	};
}

// Check that dev_ino_pair meets the LiteralType concept.
static_assert(std::is_trivially_destructible<dev_ino_pair>::value, "no trivial destructor.");
static_assert(std::is_class<dev_ino_pair>::value, "not an aggregate type");
static_assert(std::is_literal_type<dev_ino_pair>::value, "dev_ino_pair isn't trivial");

/**
 * Get the d_name field out of the passed dirent struct @p de and into a @c std::string, in as efficient manner as possible.
 *
 * @param de
 * @return
 */
inline std::string dirent_get_name(const dirent* de) noexcept ATTR_CONST ATTR_ARTIFICIAL;
inline std::string dirent_get_name(const dirent* de) noexcept
{
#if defined(_DIRENT_HAVE_D_NAMLEN)
		// struct dirent has a d_namelen field.
		std::string basename(de->d_name, de->d_namelen);
#elif defined(_DIRENT_HAVE_D_RECLEN) && defined(_D_ALLOC_NAMLEN)
		// We can cheaply determine how much memory we need to allocate for the name.
		std::string basename(de->d_name, strnlen(de->d_name, _D_ALLOC_NAMLEN(de)));
#else
		// All we have is a null-terminated d_name.
		std::string basename(de->d_name);
#endif

	// RVO should optimize this.
	return basename;
}


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
 * A more usable and portable replacement for glibc and POSIX dirname()'s.
 *
 * @param path  const ref to a path string.  Guaranteed to not be modified in any way by the function call.
 * @return A std::string representing the dirname part of @p path.  Guaranteed to be a normal std::string with which you may do
 *         whatever you can do with any other std::string.
 */
inline std::string dirname(const std::string &path) noexcept
{
	// Get a copy of the path string which ::dirname() can modify all it wants.
	char * modifiable_path = strdup(path.c_str());

	// Copy the output of dirname into a std:string.  We don't ever free the string dirname() returns
	// because it's either a static buffer, or it's a pointer to modifiable_path.  The latter we'll free below.
	std::string retval(::dirname(modifiable_path));

	free(modifiable_path);

	return retval;
}

/**
 * A more usable and portable replacement for glibc and POSIX basename()'s.
 *
 * @param path  const ref to a path string.  Guaranteed to not be modified in any way by the function call.
 * @return A std::string representing the basename part of @p path.  Guaranteed to be a normal std::string with which you may do
 *         whatever you can do with any other std::string.
 */
inline std::string basename(const std::string &path) noexcept
{
	// Get a copy of the path string which ::basename() can modify all it wants.
	char * modifiable_path = strdup(path.c_str());

	// Copy the output of dirname into a std:string.  We don't ever free the string basename() returns
	// because it's either a static buffer, or it's a pointer to modifiable_path.  The latter we'll free below.
	std::string retval(::basename(modifiable_path));

	free(modifiable_path);

	return retval;
}


/**
 * Get the current working directory's absolute pathname.
 * @note ::get_current_dir_name() under Cygwin will currently return a DOS path if this is started
 *              under the Eclipse gdb.
 */
inline std::string get_current_dir_name(void)
{
#ifdef HAVE_GET_CURRENT_DIR_NAME
	char *cur_dir_name = ::get_current_dir_name();
#else
	/// @todo Note that POSIX doesn't guarantee this will malloc the cwd string.
	char *cur_dir_name = ::getcwd(NULL, 0);
#endif
	if(cur_dir_name == nullptr)
	{
		/// @todo We probably should throw here instead.
		return ".";
	}

	std::string retval {cur_dir_name};
	free(cur_dir_name);
	return retval;
}


/// Get the home directory of the current user.  Returns an empty string if no
/// home dir can be found.
inline std::string get_home_dir_name()
{
	std::string retval;

	// First try the $HOME environment variable.
	const char * home_path = getenv("HOME");

	if(home_path == nullptr)
	{
		// No HOME variable, check the user database.
		home_path = getpwuid(getuid())->pw_dir;
	}

	if(home_path != nullptr)
	{
		// Found user's HOME dir.
		retval = home_path;
	}

	return retval;
}

/**
 * Convert @p path into an absolute file path.
 *
 * @todo Suspect this is broken on... wait for it... OSX.
 *
 * @param path
 * @return
 */
inline std::string canonicalize_file_name(const std::string &path)
{
	std::string retval;
	/// @todo Maybe prefer glibc extension canonicalize_file_name() here?
	char * fn = ::realpath(path.c_str(), nullptr);
	if(fn == nullptr)
	{
		throw FileException("realpath failed");
	}
	retval.assign(fn);
	::free(fn);
	return retval;
}

} // namespace

/**
 * Examines the given @p path and determines if it is absolute.
 *
 * @note Only POSIX-like systems currently supported.
 * @todo Expand support to Windows.
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

/**
 * Takes an absolute or relative path, possibly with trailing slashes, and removes the unnecessary trailing
 * slashes, and any unnecessary non-existent directory (e.g. "./" gets removed).
 *
 * @param path
 * @return
 */
inline std::string clean_up_path(const std::string &path) noexcept
{
	// For Posix, there are three situations we need to consider here:
	// 1. An absolute path starting with 1 or 2 slashes needs those slashes left alone.
	// 2. An absolute path with >= 3 slashes can be stripped down to 1 slash.
	// 3. Any number of slashes not at the beginning of the path should be stripped.

	auto dir = portable::dirname(path);
	auto base = portable::basename(path);
	if(dir.empty() || dir == ".")
	{
		// There is no "dir" component.  If dirname() finds no slashes in the path,
		// it returns a ".".  We don't want to prepend a "./" to the name, so we just return the basename.
		// Note that GNU grep does not appear to do this; all paths it prints look like they're dirname()+"/"+basename().
		// ag and ack however do appear to be doing this "empty dirname() elision".
		return base;
	}
	else
	{
		return dir + "/" + base;
	}
}


inline DIR* opendirat(int at_dir, const char *name)
{
	LOG(INFO) << "Attempting to open directory '" << name << "' at file descriptor " << at_dir;

	int file_fd = openat(at_dir, name, O_SEARCH | O_DIRECTORY | O_NOCTTY);
	if(file_fd < 0)
	{
		ERROR() << "openat() failed: " << LOG_STRERROR();
		//errno = 0;
		return nullptr;
	}
	DIR* d = fdopendir(file_fd);
	if(d == nullptr)
	{
		ERROR() << "fdopendir failed: " << LOG_STRERROR();
		//errno = 0;
		close(file_fd);
	}

	return d;
}

#endif /* SRC_LIBEXT_FILESYSTEM_HPP_ */
