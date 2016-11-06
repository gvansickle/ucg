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
#include <memory>

#include "integer.hpp"
#include "filesystem.hpp"


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


/**
 * The public interface to the underlying UnsynchronizedFileID instance.  This class adds thread safety.
 */
class FileID
{
private:

	using MutexType = std::mutex;  /// @todo C++17, use std::shared_mutex.  C++14, use std::shared_timed_mutex.
	using ReaderLock = std::unique_lock<MutexType>;  /// @todo C++14+, use std::shared_lock.
	using WriterLock = std::unique_lock<MutexType>;

	/// Mutex for locking in copy and move constructors and some operations.
	mutable std::mutex m_mutex;
	//ReaderLock m_reader_lock;
	//WriterLock m_writer_lock;

public:

	/// pImpl forward declaration.
	/// Not private: only because we want to do some static_assert() checks on it.
	class UnsynchronizedFileID;

	/// @name Tag types for selecting FileID() constructors when the given path is known to be relative or absolute.
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

	/// Our equivalent for AT_FDCWD, the cwd of the process.
	/// Different in that each FileID created with this constructor holds a real file handle to the "." directory.
	FileID(path_known_cwd_tag tag);
	FileID(path_known_relative_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, const struct stat *stat_buf = nullptr, FileType type = FT_UNINITIALIZED);
	FileID(path_known_relative_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, FileType type = FT_UNINITIALIZED);
	FileID(path_known_absolute_tag tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type = FT_UNINITIALIZED);
	FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname);
	FileID(const FTSENT *ftsent, bool stat_info_known_valid);
	FileID(const FileID& other);
	FileID(FileID&& other);
	/// @}

	/// Copy assignment.
	FileID& operator=(const FileID& other);

	/// Move assignment.
	FileID& operator=(FileID&& other);

	/// Destructor.
	~FileID();

	const std::string& GetBasename() const noexcept;
	const std::string& GetPath() const;

	FileDescriptor GetFileDescriptor();

	FileType GetFileType() const noexcept;
	bool IsRegularFile() const noexcept { return GetFileType() == FT_REG; };
	bool IsDir() const noexcept { return GetFileType() == FT_DIR; };

	bool IsAtFDCWD() const noexcept;

	/// @todo This should maybe be weak_ptr.
	const std::shared_ptr<FileID>& GetAtDirCRef() const noexcept;
	std::shared_ptr<FileID> GetAtDir() const noexcept;

	const std::string& GetAtDirRelativeBasename() const noexcept;

	bool IsStatInfoValid() const noexcept;

	off_t GetFileSize() const noexcept;

	blksize_t GetBlockSize() const noexcept;

	const dev_ino_pair GetUniqueFileIdentifier() const noexcept;

	dev_t GetDev() const noexcept;

	void SetDevIno(dev_t d, ino_t i) noexcept;

private:

	void UnsyncedSetStatInfo(const struct stat &stat_buf) const noexcept;

	void LazyLoadStatInfo() const;

	const std::string& UnsyncedGetPath() const;

	/// The pImpl.
	std::unique_ptr<UnsynchronizedFileID> m_data;
};

static_assert(std::is_assignable<FileID, FileID>::value, "FileID must be assignable to itself.");
static_assert(std::is_copy_assignable<FileID>::value, "FileID must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileID>::value, "FileID must be move assignable to itself.");

#endif /* SRC_LIBEXT_FILEID_H_ */
