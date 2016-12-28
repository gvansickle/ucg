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

#include <future/memory.hpp>
#include <future/shared_mutex.hpp>

#include "hints.hpp"

#include <sys/stat.h> // For the stat types.

#include <string>
#include <atomic>
#include <climits>

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
enum FileAccessMode : int
{
	FAM_UNINITIALIZED = INT_MAX,	//!< Because O_RDONLY and O_SEARCH == 0.
	FAM_RDONLY = O_RDONLY,			//!< FAM_RDONLY
	FAM_RDWR = O_RDWR,				//!< FAM_RDWR
	FAM_SEARCH = O_SEARCH			//!< FAM_SEARCH
};

/**
 * May be bitwise-or combined with FileAccesMode and each other.
 */
enum FileCreationFlag : int
{
	FCF_UNINITIALIZED = 0,
	FCF_CLOEXEC = O_CLOEXEC,    //!< FCF_CLOEXEC
	FCF_CREAT = O_CREAT,		//!< FCF_CREAT
	FCF_DIRECTORY = O_DIRECTORY,//!< FCF_DIRECTORY
	FCF_NOCTTY = O_NOCTTY,      //!< FCF_NOCTTY
	FCF_NOFOLLOW = O_NOFOLLOW,  //!< FCF_NOFOLLOW
	FCF_NOATIME = O_NOATIME,	//!< FCF_NOATIME
	FCF_NONBLOCK = O_NONBLOCK,
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

	/// Mutex for locking in copy and move constructors, other operations.
	mutable MutexType m_mutex;

public: // To allow access from impl.

	enum IsValid
	{
		NONE = 0,
		FILE_DESC = 1,
		UUID = 2, ///< i.e. dev/ino.
		STATINFO = 4,
		TYPE = 8,
		PATH = 16,
	};

private:

	mutable std::atomic_uint_fast8_t m_valid_bits { NONE };

public:

	/// pImpl forward declaration.
	/// Not private: only because we want to do some static_assert() checks on it.
	class impl;

	/// @name Tag types for selecting FileID() constructors when the given path is known to be relative or absolute.
	/// @note This is not really "tag dispatching" as commonly understood, but I don't have a way yet to disambiguate
	///       based solely on the path string.
	/// @{
	struct path_known_relative_tag {};
	struct path_known_absolute_tag {};
	struct path_known_cwd_tag {}; 	/// Our equivalent for AT_FDCWD, the cwd of the process.
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
			const struct stat *stat_buf = nullptr,
			FileType type = FT_UNINITIALIZED,
			dev_t d = static_cast<dev_t>(-1), ino_t i = 0,
			FileAccessMode fam = FAM_UNINITIALIZED, FileCreationFlag fcf = FCF_UNINITIALIZED);
	FileID(path_known_relative_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, FileType type = FT_UNINITIALIZED);
	FileID(path_known_absolute_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type = FT_UNINITIALIZED);
	FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname,
			FileAccessMode fam = FAM_UNINITIALIZED, FileCreationFlag fcf = FCF_UNINITIALIZED);
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
	 * @return Ref to a std::string containing the file's path.  Threadsafe as long as this FileID exists.
	 */
	const std::string& GetPath() const noexcept;

	const std::string& GetAbsPath() const noexcept;

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
	 * Stat the given filename at the directory represented by this.
	 *
	 * @note Only makes sense to call on FileIDs representing directories.
	 *
	 * @todo Derived class for dirs?
	 *
	 * @param name
	 * @param statbuf
	 * @param flags
	 */
	bool FStatAt(const std::string &name, struct stat *statbuf, int flags);

	FileID OpenAt(const std::string &name, FileType type, int flags);

	DIR *OpenDir();
	void CloseDir(DIR*d);

	/**
	 *
	 * @return
	 */
	const FileDescriptor& GetFileDescriptor();

	/**
	 * Return the type of file this FileID represents.  May involve stat()ing the file.
	 * @return
	 */
	FileType GetFileType() const noexcept;

	off_t GetFileSize() const noexcept;

	blksize_t GetBlockSize() const noexcept;

	const dev_ino_pair GetUniqueFileIdentifier() const noexcept;

	dev_t GetDev() const noexcept;

	void SetDevIno(dev_t d, ino_t i) noexcept;

	friend std::ostream& operator<<(std::ostream &ostrm, const FileID &fileid);

///@debug private:

	void SetStatInfo(const struct stat &stat_buf) noexcept;

	/// The pImpl.
	std::unique_ptr<impl> m_pimpl;
};

std::ostream& operator<<(std::ostream &ostrm, const FileID &fileid);

constexpr inline FileID::IsValid operator|(FileID::IsValid a, FileID::IsValid b) { return (FileID::IsValid)((int)a | (int)b) ; };

static_assert(std::is_assignable<FileID, FileID>::value, "FileID must be assignable to itself.");
static_assert(std::is_copy_assignable<FileID>::value, "FileID must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileID>::value, "FileID must be move assignable to itself.");

#endif /* SRC_LIBEXT_FILEID_H_ */
