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

#include <sys/stat.h> // For the stat types.
#include <fts.h>

#include <string>

#include "integer.hpp"
#include "filesystem.hpp"


/**
 *
 */
class FileID
{
public:
	/// @name Tag types for selecting FileID() constructors when the given path is known to be relative or absolute.
	/// @{
	struct path_known_relative_t {};
	struct path_known_absolute_t {};
	static constexpr path_known_relative_t path_known_relative = path_known_relative_t();
	static constexpr path_known_absolute_t path_known_absolute = path_known_absolute_t();
	/// @}

	/// @name Constructors.
	/// @{
	FileID() = default;
	FileID(int v) : m_path(".") {};
	FileID(path_known_relative_t tag, const FileID& at_dir_fileid, const std::string &pathname);
	FileID(path_known_absolute_t tag, const FileID& at_dir_fileid, const std::string &pathname);
	FileID(const FTSENT *ftsent);
	FileID(const FileID&) = default;
	FileID(FileID&&) = default;
	/// @}
	FileID& operator=(const FileID&) = default;
	~FileID();

	std::string GetPath() const { return m_path; };

	bool IsStatInfoValid() const noexcept { return m_stat_info_valid; };

	off_t GetFileSize() const noexcept { LazyLoadStatInfo(); return m_size; };

	blksize_t GetBlockSize() const noexcept
	{
		LazyLoadStatInfo();
		return m_block_size;
	};

private:

	void LazyLoadStatInfo() const;

	/// The path to this file.
	std::string m_path;

	/// @name Info normally gathered from a stat() call.
	///@{

	/// Indicator of whether the stat info is valid or not.
	mutable bool m_stat_info_valid { false };

	mutable dev_ino_pair m_unique_file_identifier;

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

#endif /* SRC_LIBEXT_FILEID_H_ */
