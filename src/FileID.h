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

#include <libext/filesystem.hpp>


/*
 *
 */
class FileID
{
public:
	FileID() = default;
	FileID(const FTSENT *ftsent);
	~FileID();

	std::string get_path() const { return m_path; };

private:

	std::string m_path;

	/// @name Info normally gathered from a stat() call.
	///@{
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
