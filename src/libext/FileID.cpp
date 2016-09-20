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

#include <config.h>

#include "FileID.h"

#include <sys/stat.h>
#include <fts.h>


FileID::FileID(path_known_relative_t tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename) : FileID(at_dir_fileid, basename)
{
	/// @note Taking basename by value since we are always storing it.
	/// @todo Needs full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.
}

FileID::FileID(path_known_absolute_t tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname) : m_at_dir(at_dir_fileid), m_path(pathname)
{
	/// @note Taking pathname by value since we are always storing it.
	/// @todo Needs full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.
}

FileID::FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname) : m_at_dir(at_dir_fileid)
{
	/// @todo Needs full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.

	if(is_pathname_absolute(pathname))
	{
		m_path = pathname;
	}
	else
	{
		m_basename = pathname;
	}
}

FileID::FileID(const dirent *de)
{

}

FileID::FileID(const FTSENT *ftsent): m_path(ftsent->fts_path, ftsent->fts_pathlen)
{
	// Initialize the stat fields if possible.
	if(ftsent->fts_statp != nullptr)
	{
		m_stat_info_valid = true;
		m_unique_file_identifier = dev_ino_pair(ftsent->fts_statp->st_dev, ftsent->fts_statp->st_ino);
		m_size = ftsent->fts_statp->st_size;
		m_block_size = ftsent->fts_statp->st_blksize;
		m_blocks = ftsent->fts_statp->st_blocks;
	}
}

FileID::~FileID()
{
}

const std::string& FileID::GetPath() const
{
	if(m_path.empty())
	{
		// Build the full path.
		m_path = m_at_dir->GetPath() + '/' + m_basename;
	}

	return m_path;
}

void FileID::LazyLoadStatInfo() const
{
	if(IsStatInfoValid())
	{
		// Already set.
		return;
	}

	// We don't have stat info and now we need it.
	// Get it from the filename.
	struct stat stat_buf;
	if(stat(GetPath().c_str(), &stat_buf) != 0)
	{
		// Error.
		/// @todo
	}
	else
	{
		m_stat_info_valid = true;
		m_unique_file_identifier = dev_ino_pair(stat_buf.st_dev, stat_buf.st_ino);
		m_size = stat_buf.st_size;
		m_block_size = stat_buf.st_blksize;
		m_blocks = stat_buf.st_blocks;
	}
}
