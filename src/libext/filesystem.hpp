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

#include <cassert>
#include <cstdio>  // For perror() on FreeBSD.
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
#include <fts.h>

/// @note Because we included libgen.h above, we shouldn't get the GNU version from this #include of string.h.
#include <string.h>
#include <cstdlib>   // For free().
#include <string>
#include <iterator>   // For std::distance().
#include <future/type_traits.hpp>
#include <future/memory.hpp>

#include "integer.hpp"
#include "../Logger.h"


/// @name Take care of some portability issues.
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
/// @}


/**
 * Class intended to abstract the concept of a UUID for a file or directory.
 * @todo Currently only supports POSIX-like OSes, and not necessarily all filesystems.  Needs to be expanded.
 */
struct dev_ino_pair
{
	dev_ino_pair() = default;
	dev_ino_pair(dev_t d, ino_t i) noexcept : m_dev(d), m_ino(i) { };

	constexpr bool operator<(const dev_ino_pair& other) const noexcept { return m_dev < other.m_dev || m_ino < other.m_ino; };

	constexpr bool operator==(dev_ino_pair other) const noexcept { return m_dev == other.m_dev && m_ino == other.m_ino; };

	constexpr bool empty() const noexcept { return m_dev == 0 && m_ino == 0; };
private:
	friend struct std::hash<dev_ino_pair>;

	//dev_ino_pair_type m_val { 0 };
	dev_t m_dev {0};
	ino_t m_ino {0};
};

namespace std
{
	// Inject a specialization of std::hash<> into std:: for dev_ino_pair.
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

/**
 * Get the d_name field out of the passed dirent struct #de and into a std::string, in as efficient manner as posible.
 *
 * @param de
 * @return
 */
inline std::string dirent_get_name(const dirent* de) noexcept
{
#if defined(_DIRENT_HAVE_D_NAMLEN)
		// struct dirent has a d_namelen field.
		std::string basename(de->d_name, de->d_namelen);
#elif defined(_DIRENT_HAVE_D_RECLEN) && defined(_D_ALLOC_NAMLEN)
		// We can cheaply determine how much memory we need to allocate for the name.
		/// @todo May not have a strnlen(). // std::string basename(_D_ALLOC_NAMLEN(de), '\0');
		std::string basename(de->d_name, strnlen(de->d_name, _D_ALLOC_NAMLEN(de)));
#else
		// All we have is a null-terminated d_name.
		std::string basename(de->d_name);
#endif

	// RVO should optimize this.
	return basename;
}


#if 0 /// @todo
constexpr int cm_invalid_file_descriptor = -987;
struct FileDescriptorDeleter
{
	void operator()(int *fd) const noexcept
	{
		if(*fd >= 0)
		{
			LOG(INFO) << "Closing file descriptor " << *fd;
			close(*fd);
		}
		delete fd;
	}
};
//using FileDescriptor = std::shared_ptr<int>;
using FileDescriptor = std::unique_ptr<int, FileDescriptorDeleter>;

inline FileDescriptor make_unique_fd(int fd) ATTR_ARTIFICIAL;
inline FileDescriptor make_unique_fd(int fd)
{
	return std::unique_ptr<int, FileDescriptorDeleter>(new int(fd));
}

inline std::shared_ptr<int> make_shared_fd(int fd) ATTR_ARTIFICIAL;
inline std::shared_ptr<int> make_shared_fd(int fd)
{
	return FileDescriptor(new int(fd), FileDescriptorDeleter());
}

#else

/**
 * Wrapper for C's 'int' file descriptor.
 * This class only adds C++ RAII abilities and correct move semantics to a file descriptor.
 */
class FileDescriptor
{
	using MutexType = std::mutex;  /// @todo C++17, use std::shared_mutex.  C++14, use std::shared_timed_mutex.
	using ReaderLock = std::unique_lock<MutexType>;  /// @todo C++14+, use std::shared_lock.
	using WriterLock = std::unique_lock<MutexType>;

	/// Mutex for locking in copy and move constructors and some operations.
	mutable MutexType m_mutex;

public:
	FileDescriptor() noexcept = default;

	explicit FileDescriptor(int fd) noexcept
	{
		WriterLock wl(m_mutex);
		m_file_descriptor = fd;
	};

	/// Copy constructor will dup the other's file descriptor.
	FileDescriptor(const FileDescriptor &other) noexcept
	{
		ReaderLock rl(other.m_mutex);

		if(!other.unlocked_empty())
		{
			m_file_descriptor = dup(other.m_file_descriptor);
		}
		else
		{
			// Other has an invalid fd, just copy the value.
			m_file_descriptor = other.m_file_descriptor;
		}
	}

	/// Move constructor.  Ownership of the fd will be transferred from other to this.
	/// For a move, the #other FileDescriptor has to be invalidated.  Otherwise,
	/// when it is destroyed, it will close the file, which it no longer owns.
	FileDescriptor(FileDescriptor&& other) noexcept
	{
		WriterLock wl(other.m_mutex);

		m_file_descriptor = other.m_file_descriptor;

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
		//WriterLock wl(m_mutex);
		if(!unlocked_empty())
		{
			close(m_file_descriptor);
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

			if(!unlocked_empty())
			{
				close(m_file_descriptor);
			}

			if(other.unlocked_empty())
			{
				// Other fd isn't valid, just copy it.
				m_file_descriptor = other.m_file_descriptor;
			}
			else
			{
				m_file_descriptor = dup(other.m_file_descriptor);
				if((m_file_descriptor < 0) && (m_file_descriptor != AT_FDCWD))
				{
					perror("dup");
				}
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

			// Step 1: Release any resources this owns.
			if(!unlocked_empty())
			{
				close(m_file_descriptor);
			}

			// Step 2: Take other's resources.
			m_file_descriptor = other.m_file_descriptor;

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

	/// Returns true if this FileDescriptor isn't a valid file descriptor.
	inline bool empty() const noexcept
	{
		ReaderLock rl(m_mutex);
		return unlocked_empty();
	}

private:
	static constexpr int cm_invalid_file_descriptor = -987;

	int m_file_descriptor { cm_invalid_file_descriptor };

	inline bool unlocked_empty() const noexcept { return m_file_descriptor < 0; };
};

inline FileDescriptor make_shared_fd(int fd)
{
	return FileDescriptor(fd);
}

#endif


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
 * @return  A std::string representing the dirname part of #path.  Guaranteed to be a normal std::string with which you may do
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

/**
 * A more usable and portable replacement for glibc and POSIX basename().
 *
 * @param path  const ref to a path string.  Guaranteed to not be modified in any way by the function call.
 * @return  A std::string representing the basename part of path.  Guaranteed to be a normal std::string with which you may do
 *          whatever you can do with any other std::string.
 */
inline std::string basename(const std::string &path)
{
	// Get a copy of the path string which dirname() can modify all it wants.
	char * modifiable_path = strdup(path.c_str());

	// Copy the output of dirname into a std:string.  We don't ever free the string dirname() returns
	// because it's either a static buffer, or it's a pointer to modifiable_path.  The latter we'll free below.
	std::string retval(::basename(modifiable_path));

	free(modifiable_path);

	return retval;
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

/**
 * Takes an absolute or relative path, possibly with trailing slashes, and removes the unnecessary trailing
 * slashes.
 *
 * @param path
 * @return
 */
inline std::string canonicalize_any_path(const std::string &path)
{

	/// @todo The below will add e.g. "./" to a path with no dir.  We need that to not happen.
	return path;
#if 0
	std::string::const_reverse_iterator rbegin = path.rbegin();

	// For Posix, there are three situations we need to consider here:
	// 1. An absolute path starting with 1 or 2 slashes needs those slashes left alone.
	// 2. An absolute path with >= 3 slashes can be stripped down to 1 slash.
	// 3. Any number of slashes not at the beginning of the path should be stripped.

	auto dir = portable::dirname(path);
	auto base = portable::basename(path);
	if(!dir.empty() && dir.find_first_of("/\\", dir.length()-1) != std::string::npos)
	{
		// Ends in a slash (possibly root), just concat.
		return dir + base;
	}
	else
	{
		return dir + "/" + base;
	}
#endif
}


inline DIR* opendirat(int at_dir, const char *name)
{
	LOG(INFO) << "Attempting to open directory '" << name << "' at file descriptor " << at_dir;
	constexpr int openat_dir_search_flags = O_SEARCH ? O_SEARCH : O_RDONLY;

	int file_fd = openat(at_dir, name, openat_dir_search_flags | O_DIRECTORY | O_NOCTTY);
	if(file_fd < 0)
	{
		ERROR() << "openat() failed: " << LOG_STRERROR();
		errno = 0;
	}
	DIR* d = fdopendir(file_fd);
	if(d == nullptr)
	{
		ERROR() << "fdopendir failed: " << LOG_STRERROR();
		errno = 0;
	}

	return d;
}


/// @name FTS helpers.
/// @{


inline int64_t ftsent_level(const FTSENT* p)
{
	// We store the "real level" of the parent directory in the fts_number member.
	return p->fts_level + p->fts_number;
}

/**
 * Returns just the basename of the file represented by #p.
 * @param p
 * @return
 */
inline std::string ftsent_name(const FTSENT* p)
{
	if(p != nullptr)
	{
		return std::string(p->fts_name, p->fts_namelen);
	}
	else
	{
		return "<nullptr>";
	}
}

/**
 * Returns the full path (dirname + basename) of the file/dir represented by #p.
 * @param p
 * @return
 */
inline std::string ftsent_path(const FTSENT* p)
{
	//return ftsent_name(p);
	if(p != nullptr)
	{
		std::string retval;
		if(p->fts_parent != nullptr)
		{
			if(p->fts_parent->fts_pathlen > 0)
			{
				retval.assign(p->fts_parent->fts_path, p->fts_parent->fts_pathlen);
				retval += '/';
			}
		}
		retval.append(p->fts_name, p->fts_namelen);
		return retval;
	}
	else
	{
		return "<nullptr>";
	}
}

///@}

#endif /* SRC_LIBEXT_FILESYSTEM_HPP_ */
