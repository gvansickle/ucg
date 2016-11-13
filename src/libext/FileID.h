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

#ifndef SRC_LIBEXT_FILEID_H_
#define SRC_LIBEXT_FILEID_H_

#include <config.h>

#include "hints.hpp"

#include <sys/stat.h> // For the stat types.
#include <fts.h>

#include <string>
#include <atomic>
#include <future/memory.hpp>
#include <future/shared_mutex.hpp>

#include "integer.hpp"
#include "filesystem.hpp"
#include "FileDescriptor.hpp"


// Forward declarations.
struct dirent;
class FileID;  // UnsynchronizedFileID keeps a ptr to its parent directory's FileID.

/// File Types enum.
enum FileType
{
	FT_UNINITIALIZED,
	FT_UNKNOWN,
	FT_REG,
	FT_DIR,
	FT_SYMLINK,
	FT_STAT_FAILED
};
/// Stream insertion operator for FileType enum.
inline std::ostream& operator<<(std::ostream& out, const FileType value){
    const char* s = 0;
#define M_ENUM_CASE(p) case(p): s = #p; break;
    switch(value){
        M_ENUM_CASE(FT_UNINITIALIZED);
        M_ENUM_CASE(FT_UNKNOWN);
        M_ENUM_CASE(FT_REG);
        M_ENUM_CASE(FT_DIR);
		M_ENUM_CASE(FT_SYMLINK);
		M_ENUM_CASE(FT_STAT_FAILED);
    }
#undef M_ENUM_CASE

    return out << s;
}

/**
 * Only one may be specified.
 */
enum FileAccessMode
{
	FAM_RDONLY = O_RDONLY,//!< FAM_RDONLY
	FAM_RDWR = O_RDWR,    //!< FAM_RDWR
	FAM_SEARCH = O_SEARCH //!< FAM_SEARCH
};

/**
 * May be bitwise-or combined with FileAccesMode and each other.
 */
enum FileCreationFlag : int
{
	FCF_CLOEXEC = O_CLOEXEC,    //!< FCF_CLOEXEC
	FCF_CREAT = O_CREAT,		//!< FCF_CREAT
	FCF_DIRECTORY = O_DIRECTORY,//!< FCF_DIRECTORY
	FCF_NOCTTY = O_NOCTTY,      //!< FCF_NOCTTY
	FCF_NOFOLLOW = O_NOFOLLOW,  //!< FCF_NOFOLLOW
	FCF_NOATIME = O_NOATIME
};

/// Bitwise-or operator for FileCreationFlag.
/// @note Yeah, I didn't realize this was necessary for non-class enums in C++ either.  I've been writing too much C....
constexpr inline FileCreationFlag operator|(FileCreationFlag a, FileCreationFlag b)
{
	return static_cast<FileCreationFlag>(static_cast<std::underlying_type<FileCreationFlag>::type>(a)
			| static_cast<std::underlying_type<FileCreationFlag>::type>(b));
}


/**
 * The public interface to the underlying UnsynchronizedFileID instance.  This class adds thread safety.
 */
class FileID
{
private:

	/// We're bringing this mutex type in from the future: @see <future/shared_mutex.hpp>.
	/// Under C++17, this is really a std::shared_mutex.
	/// In C++14, it's a std::shared_timed_mutex.
	/// In C++11, it's a regular std::mutex.
	using MutexType = std::shared_mutex;
	  /// Likewise with this type.  In C++14+, it's a real std::shared_lock, else it's a std::unique_lock.
	using ReaderLock = std::shared_lock<MutexType>;
	using WriterLock = std::unique_lock<MutexType>;

#if 0 /// @todo For double-checked locking on m_path.
	mutable std::atomic<std::string*> m_atomic_path_ptr { nullptr };
#endif

	/// Mutex for locking in copy and move constructors and some operations.
	mutable MutexType m_mutex;

public:

	/// pImpl forward declaration.
	/// Not private: only because we want to do some static_assert() checks on it.
	class impl;

	/// @name Tag types for selecting FileID() constructors when the given path is known to be relative or absolute.
	/// @note This is not really "tag dispatching" as commonly understood, but I don't have a way yet to disambiguate
	///       based solely on the path string.
	/// @{
	struct path_type_tag {};
	struct path_known_relative_tag : path_type_tag {};
	struct path_known_absolute_tag : path_type_tag {};
	struct path_known_cwd_tag : path_type_tag {}; 	/// Our equivalent for AT_FDCWD, the cwd of the process.
	static constexpr path_known_relative_tag path_known_relative = path_known_relative_tag();
	static constexpr path_known_absolute_tag path_known_absolute = path_known_absolute_tag();
	static constexpr path_known_cwd_tag path_known_cwd = path_known_cwd_tag();
	/// @}

	/// @name Constructors.
	/// @{
	FileID();
	FileID(const FileID& other);
	FileID(FileID&& other);

	/// Our equivalent for AT_FDCWD, the cwd of the process.
	/// Different in that each FileID created with this constructor holds a real file handle to the "." directory.
	FileID(path_known_cwd_tag tag);
	FileID(path_known_relative_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename,
			const struct stat *stat_buf = nullptr, FileType type = FT_UNINITIALIZED);
	FileID(path_known_relative_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, FileType type = FT_UNINITIALIZED);
	FileID(path_known_absolute_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type = FT_UNINITIALIZED);
	FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname);
	FileID(const FTSENT *ftsent, bool stat_info_known_valid);

	/// @}

	/// Copy assignment.
	FileID& operator=(const FileID& other);

	/// Move assignment.
	FileID& operator=(FileID&& other);

	/// Destructor.
	~FileID();

	std::string GetBasename() const noexcept;

	/**
	 * Returns the "full path" of the file.  May be absolute or relative to the root AT dir.
	 * @return A std::string object (not a reference) containing the file's path.
	 */
	std::string GetPath() const noexcept;

	/**
	 * This is essentially a possibly-deferred "open()" for this class.
	 *
	 * @post GetFileDescriptor() will return a FileDescriptor to the file with the given access mode and creation flags.
	 *
	 * @param fam
	 * @param fcf
	 */
	void SetFileDescriptorMode(FileAccessMode fam, FileCreationFlag fcf);

	/**
	 *
	 * @return
	 */
	FileDescriptor GetFileDescriptor();

	/**
	 * Return the type of file this FileID represents.  May involve stat()ing the file.
	 * @return
	 */
	FileType GetFileType() const noexcept;
	bool IsRegularFile() const noexcept { return GetFileType() == FT_REG; };
	bool IsDir() const noexcept { return GetFileType() == FT_DIR; };

	std::shared_ptr<FileID> GetAtDir() const noexcept;

	const std::string& GetAtDirRelativeBasename() const noexcept;

	bool IsStatInfoValid() const noexcept;

	off_t GetFileSize() const noexcept;

	blksize_t GetBlockSize() const noexcept;

	const dev_ino_pair GetUniqueFileIdentifier() const noexcept;

	dev_t GetDev() const noexcept;

	void SetDevIno(dev_t d, ino_t i) noexcept;

///@debug private:

	void SetStatInfo(const struct stat &stat_buf) noexcept;

	/// The pImpl.
	std::unique_ptr<impl> m_pimpl;
};

static_assert(std::is_assignable<FileID, FileID>::value, "FileID must be assignable to itself.");
static_assert(std::is_copy_assignable<FileID>::value, "FileID must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileID>::value, "FileID must be move assignable to itself.");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Factorization of the FileID class.  This is the unsynchronized "pImpl" part which holds all the data.
 * It does not concern itself with concurrency issues with respect to copying, moving, or assigning.
 * As such, its interface is not intended to be directly exposed to the world.
 * @see FileID
 */
class FileID::impl
{
public:
	impl() = default;
	impl(const impl& other) = default;
	impl(impl&& other) = default;

	/// Copy assignment.
	impl& operator=(const impl &other) = default;

	/// Move assignment.
	impl& operator=(impl&& other) = default;

	/// @name Various non-default constructors.
	/// @{
	impl(const FTSENT *ftsent, bool stat_info_known_valid);
	impl(std::shared_ptr<FileID> at_dir_fileid, std::string pathname);
	impl(std::shared_ptr<FileID> at_dir_fileid, std::string basename, std::string pathname,
			const struct stat *stat_buf = nullptr, FileType type = FT_UNINITIALIZED);
	///@}

	/// Default destructor.
	~impl() = default;

	const std::string& GetBasename() const noexcept;

	/// Create and cache this FileID's path.  Recursively visits its parent directories to do so.
	void GetPath() const;

	/**
	 * Determine if this file's full path has been determined and cached in the m_path std::string member.
	 * @return
	 */
	bool IsPathResolved() const { return !m_path.empty(); };

	FileDescriptor GetFileDescriptor();

	FileType GetFileType() const noexcept
	{
		if(m_file_type == FT_UNINITIALIZED)
		{
			// We don't know the file type yet.  We'll have to get it from a stat() call.
			LazyLoadStatInfo();
		}

		return m_file_type;
	};

//private:

	void LazyLoadStatInfo() const
	{
		if(m_stat_info_valid)
		{
			// Already set.
			return;
		}

		// We don't have stat info and now we need it.
		// Get it from the filename.
		if(!m_at_dir)
		{
			throw std::runtime_error("should have an at-dir");
		}

		struct stat stat_buf;
		if(fstatat(m_at_dir->GetFileDescriptor().GetFD(), m_basename.c_str(), &stat_buf, AT_NO_AUTOMOUNT) != 0)
		{
			// Error.
			m_file_type = FT_STAT_FAILED;
			LOG(INFO) << "fstatat() failed: " << LOG_STRERROR();
			// Note: We don't clear errno here, we want to be able to look at it in the caller.
			//errno = 0;
		}
		else
		{
			SetStatInfo(stat_buf);
		}
	}

	void SetStatInfo(const struct stat &stat_buf) const noexcept;

	std::shared_ptr<FileID> GetAtDir() const noexcept { return m_at_dir; };

	const std::string& GetAtDirRelativeBasename() const noexcept;

	bool IsStatInfoValid() const noexcept { return m_stat_info_valid; };

	off_t GetFileSize() const noexcept { LazyLoadStatInfo(); return m_size; };

	blksize_t GetBlockSize() const noexcept
	{
		LazyLoadStatInfo();
		return m_block_size;
	};

	const dev_ino_pair GetUniqueFileIdentifier() const noexcept { if(!m_unique_file_identifier.empty()) { LazyLoadStatInfo(); }; return m_unique_file_identifier; };

	dev_t GetDev() const noexcept { if(m_dev == static_cast<dev_t>(-1)) { LazyLoadStatInfo(); }; return m_dev; };
	void SetDevIno(dev_t d, ino_t i) noexcept;



	/// Shared pointer to the directory this FileID is in.
	/// The constructors ensure that this member always exists and is valid.
	std::shared_ptr<FileID> m_at_dir;

	/// The basename of this file.
	/// We define this somewhat differently here: This is either:
	/// - The full absolute path, or
	/// - The path relative to m_at_dir, which may consist of more than one path element.
	/// In any case, it is always equal to the string passed into the constructor, and this will always exist and is valid.
	std::string m_basename;

	/// The absolute path to this file.
	/// This will be lazily evaluated when needed, unless an absolute path is passed in to the constructor.
	mutable std::string m_path;

	mutable int m_open_flags { 0 };
	mutable FileDescriptor m_file_descriptor;

	/// @name Info normally gathered from a stat() call.
	///@{

	/// Indicator of whether the stat info is valid or not.
	mutable bool m_stat_info_valid { false };

	mutable FileType m_file_type { FT_UNINITIALIZED };

	mutable dev_ino_pair m_unique_file_identifier;

	mutable dev_t m_dev { static_cast<dev_t>(-1) };

	/// File size in bytes.
	mutable off_t m_size { 0 };

	/// The preferred I/O block size for this file.
	/// @note GNU libc documents the units on this as bytes.
	mutable blksize_t m_block_size { 0 };

	/// Number of blocks allocated for this file.
	/// @note POSIX doesn't define the units for this.  Linux is documented to use 512-byte units, as is GNU libc.
	mutable blkcnt_t m_blocks { 0 };
	///@}
};

/// @name Compile-time invariants for the UnsynchronizedFileID class.
/// @{
static_assert(std::is_assignable<FileID::impl, FileID::impl>::value, "UnsynchronizedFileID must be assignable to itself.");
static_assert(std::is_copy_assignable<FileID::impl>::value, "UnsynchronizedFileID must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileID::impl>::value, "UnsynchronizedFileID must be move assignable to itself.");
/// @}



/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif /* SRC_LIBEXT_FILEID_H_ */
