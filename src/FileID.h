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

#ifndef SRC_FILEID_H_
#define SRC_FILEID_H_

#include <config.h>

#include <sys/stat.h> // For the stat types.
#include <fts.h>

#include <string>

#include <libext/integer.hpp>
#include <libext/filesystem.hpp>


/*
 *
 */
class FileID
{
public:
	FileID() = default;
	FileID(const FTSENT *ftsent);
	FileID(const FileID&) = default;
	FileID& operator=(const FileID&) = default;
	FileID(FileID&&) = default;
	~FileID();

	std::string get_path() const { return m_path; };

	bool IsStatInfoValid() const noexcept { return m_stat_info_valid; };

	off_t GetFileSize() const noexcept { return m_size; };

	blksize_t GetBlockSize() const noexcept
	{
		// Note: per info here:
		// http://stackoverflow.com/questions/34498825/io-blksize-seems-just-return-io-bufsize
		// https://github.com/coreutils/coreutils/blob/master/src/ioblksize.h#L23-L57
		// http://unix.stackexchange.com/questions/245499/how-does-cat-know-the-optimum-block-size-to-use
		// ...it seems that as of ~2014, experiments show the minimum I/O size should be >=128KB.
		// *stat() seems to return 4096 in all my experiments so far, so we'll clamp it to a min of 128KB and a max of
		// something not unreasonable, e.g. 1M.
		auto retval = clamp(m_block_size, static_cast<blksize_t>(0x20000), static_cast<blksize_t>(0x100000));
		return retval;
	};

private:

	std::string m_path;

	/// @name Info normally gathered from a stat() call.
	///@{

	/// Indicator of whether the stat info is valid or not.
	bool m_stat_info_valid { false };

	dev_ino_pair m_unique_file_identifier;

	/// File size in bytes.
	off_t m_size { 0 };

	/// The preferred I/O block size for this file.
	/// @note GNU libc documents the units on this as bytes.
	blksize_t m_block_size { 0 };

	/// Number of blocks allocated for this file.
	/// @note POSIX doesn't define the units for this.  Linux is documented to use 512-byte units, as is GNU libc.
	blkcnt_t m_blocks { 0 };
	///@}

};

#endif /* SRC_FILEID_H_ */
