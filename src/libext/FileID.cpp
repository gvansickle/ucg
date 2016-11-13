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

#include <fcntl.h> // For AT_FDCWD, AT_NO_AUTOMOUNT
#include <unistd.h> // For close().
#include <sys/stat.h>
#include <fts.h>

#include <future/memory.hpp> // For std::make_unique<>
#include <atomic>

//////////////////////////////////
///


FileID::UnsynchronizedFileID::UnsynchronizedFileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname)
	: m_at_dir(at_dir_fileid), m_basename(pathname)
{
	/// @note Taking pathname by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.

	LOG(INFO) << "2-param const., m_basename=" << m_basename << ", m_at_dir=" << m_at_dir->m_pimpl->m_path;

	if(is_pathname_absolute(pathname))
	{
		// Save an expensive recursive call to m_at_dir->GetPath() in the future.
		m_path = pathname;
	}
}

FileID::UnsynchronizedFileID::UnsynchronizedFileID(std::shared_ptr<FileID> at_dir_fileid, std::string basename, std::string pathname,
		const struct stat *stat_buf, FileType type)
		: m_at_dir(at_dir_fileid), m_basename(basename), m_path(pathname), m_file_type(type)
{
	if(stat_buf != nullptr)
	{
		SetStatInfo(*stat_buf);
	}
}

const std::string& FileID::UnsynchronizedFileID::GetBasename() const noexcept
{
	return m_basename;
};

FileDescriptor FileID::UnsynchronizedFileID::GetFileDescriptor()
{
	/// @todo This still needs rethinking.  I think.

	if(m_file_descriptor.empty())
	{
		// File hasn't been opened.

		/// @todo This is ugly, needs to be moved out of here.
		if(m_open_flags == 0)
		{
			m_open_flags = O_SEARCH | O_DIRECTORY | O_NOCTTY;
		}

		m_file_descriptor = make_shared_fd(openat(m_at_dir->GetFileDescriptor().GetFD(), GetBasename().c_str(), m_open_flags));

		if(m_file_descriptor.empty())
		{
			/// Try to figure out what happened and throw an exception.
			if(m_file_descriptor.GetFD() == -1)
			{
				// Couldn't open the file, throw exception.
				int temp_errno = errno;
				errno = 0;
				throw std::system_error(temp_errno, std::generic_category());
			}
			else
			{
				throw std::runtime_error("Bad fd");
			}
		}
	}

	return m_file_descriptor;
}


const std::string& FileID::UnsynchronizedFileID::GetAtDirRelativeBasename() const noexcept
{
	if(m_basename.empty())
	{
		throw std::runtime_error("basename was empty");
	}
	return m_basename;
}

void FileID::UnsynchronizedFileID::SetDevIno(dev_t d, ino_t i) noexcept
{
	m_dev = d;
	m_unique_file_identifier = dev_ino_pair(d, i);
}

void FileID::UnsynchronizedFileID::SetStatInfo(const struct stat &stat_buf) const noexcept
{
	m_stat_info_valid = true;

	// Determine file type.
	if(S_ISREG(stat_buf.st_mode))
	{
		m_file_type = FT_REG;
	}
	else if(S_ISDIR(stat_buf.st_mode))
	{
		m_file_type = FT_DIR;
	}
	else if(S_ISLNK(stat_buf.st_mode))
	{
		m_file_type = FT_SYMLINK;
	}
	else
	{
		// Those are the only types we know or care about.
		m_file_type = FT_UNKNOWN;
	}

	m_dev = stat_buf.st_dev;
	m_unique_file_identifier = dev_ino_pair(stat_buf.st_dev, stat_buf.st_ino);
	m_size = stat_buf.st_size;
	m_block_size = stat_buf.st_blksize;
	m_blocks = stat_buf.st_blocks;
}

void FileID::UnsynchronizedFileID::GetPath() const
{
	// Do we not have a full path already?
	/// @todo probably don't need this check anymore.
	if(m_path.empty())
	{
		// No.  Build the full path.
		auto at_path = m_at_dir->GetPath();
		if(at_path != ".")
		{
			//m_path.reserve(at_path.size() + m_basename.size() + 2);
			m_path = at_path + "/" + m_basename;
		}
		else
		{
			m_path = m_basename;
		}
	}
}

/// //////////////////////////////



// Default constructor.
// Note that it's defined here in the cpp vs. in the header because it needs to be able to see the full definition of UnsynchronizedFileID.
FileID::FileID() //: FileID(path_known_cwd)
{
	LOG(DEBUG) << "Default constructor called";
}

// Copy constructor.
FileID::FileID(const FileID& other) : m_pimpl((ReaderLock(other.m_mutex), std::make_unique<FileID::UnsynchronizedFileID>(*other.m_pimpl)))
{
	LOG(DEBUG) << "Copy constructor called";
	if(!m_pimpl)
	{
		throw std::runtime_error("no pimpl on copy");
	}
};

// Move constructor.
FileID::FileID(FileID&& other) : m_pimpl((WriterLock(other.m_mutex), std::move(other.m_pimpl)))
{
	LOG(DEBUG) << "Move constructor called";
	if(!m_pimpl)
	{
		throw std::runtime_error("no pimpl on move");
	}
};

FileID::FileID(path_known_cwd_tag)
	: m_pimpl(std::make_unique<FileID::UnsynchronizedFileID>(nullptr, ".", ".", nullptr, FT_DIR))
{
	m_pimpl->m_file_descriptor = make_shared_fd(open(".", O_SEARCH | O_DIRECTORY | O_NOCTTY));
}

FileID::FileID(path_known_relative_tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, FileType type)
	: m_pimpl(std::make_unique<FileID::UnsynchronizedFileID>(at_dir_fileid, basename, "", nullptr, type))
{
	/// @note Taking basename by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.
}

FileID::FileID(path_known_absolute_tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type)
	: m_pimpl(std::make_unique<FileID::UnsynchronizedFileID>(at_dir_fileid, pathname /*==basename*/, pathname, nullptr, type))
{
	/// @note Taking pathname by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.
}

FileID::FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname)
	: m_pimpl(std::make_unique<FileID::UnsynchronizedFileID>(at_dir_fileid, pathname))
{

}

FileID::FileID(path_known_relative_tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, const struct stat *stat_buf, FileType type)
	: FileID::FileID(path_known_relative, at_dir_fileid, basename, type)
{
	// basename is a file relative to at_dir_fileid, and we also have stat info for it.
	if(stat_buf != nullptr)
	{
		m_pimpl->SetStatInfo(*stat_buf);
	}
}

#if 0
FileID::FileID(const FTSENT *ftsent, bool stat_info_known_valid)
	: m_path(ftsent->fts_path, ftsent->fts_pathlen)
{
	// Initialize the stat fields if possible.
	if(stat_info_known_valid)
	{
		m_stat_info_valid = true;
		m_unique_file_identifier = dev_ino_pair(ftsent->fts_statp->st_dev, ftsent->fts_statp->st_ino);
		m_size = ftsent->fts_statp->st_size;
		m_block_size = ftsent->fts_statp->st_blksize;
		m_blocks = ftsent->fts_statp->st_blocks;
	}
}
#endif

FileID& FileID::operator=(const FileID& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		ReaderLock other_lock(other.m_mutex, std::defer_lock);
		std::lock(this_lock, other_lock);
		m_pimpl = std::make_unique<FileID::UnsynchronizedFileID>(*other.m_pimpl);
	}
	return *this;
};

FileID& FileID::operator=(FileID&& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		WriterLock other_lock(other.m_mutex, std::defer_lock);
		std::lock(this_lock, other_lock);

		m_pimpl = std::move(other.m_pimpl);
	}
	return *this;
};

FileID::~FileID()
{
	// Don't lock during destruction.
	// If anyone was trying to read or write us, they'd have to have (possibly shared) ownership (right?),
	// and hence we wouldn't be getting destroyed.
}

std::string FileID::GetBasename() const noexcept
{
	ReaderLock rl(m_mutex);
	return m_pimpl->GetBasename();
};


std::string FileID::GetPath() const noexcept
{
#if 0 /// @todo
	// Use double-checked locking to build up the path and cache it in m_path.
	if(false)
	{
		//std::call_once(m_once_flag_path, [this]{m_pimpl->GetPath();});
		auto path_ptr = m_atomic_path_ptr.load(std::memory_order_relaxed);
		std::atomic_thread_fence(std::memory_order_acquire);
		if(path_ptr == nullptr)
		{
			// First check says we don't have an m_path yet.
			WriterLock wl(m_mutex);
			// One more try.
			path_ptr = m_atomic_path_ptr.load(std::memory_order_relaxed);
			if(path_ptr == nullptr)
			{
				// Still no cached m_path.  We'll have to do the heavy lifting.
				m_pimpl->GetPath();
				path_ptr = &(m_pimpl->m_path);
				std::atomic_thread_fence(std::memory_order_release);
				m_atomic_path_ptr.store(path_ptr, std::memory_order_relaxed);
			}
		}
		return *path_ptr;
	}
	else
	{
#endif
		{
			ReaderLock rl(m_mutex);

			if(m_pimpl->IsPathResolved())
			{
				// m_path has already been lazily evaluated and is available in a std::string.
				return m_pimpl->m_path;
			}
		}
		// Else, we need to get a write lock, and evaluate the path.

		WriterLock wl(m_mutex);
		m_pimpl->GetPath();
#if 0
	}
#endif

	return m_pimpl->m_path;
}

std::shared_ptr<FileID> FileID::GetAtDir() const noexcept
{
	// Only need reader lock here, m_pimpl->m_at_dir will always exist.
	ReaderLock rl(m_mutex);
	return m_pimpl->GetAtDir();
};

const std::string& FileID::GetAtDirRelativeBasename() const noexcept
{
	// Only need reader lock here, m_pimpl->m_at_dir and its m_basename will always exist.
	ReaderLock rl(m_mutex);
	return m_pimpl->GetAtDirRelativeBasename();
}

bool FileID::IsStatInfoValid() const noexcept
{
	WriterLock rl(m_mutex);
	return m_pimpl->IsStatInfoValid();
};

FileType FileID::GetFileType() const noexcept
{
	WriterLock rl(m_mutex);

	return m_pimpl->GetFileType();
}

off_t FileID::GetFileSize() const noexcept
{
	WriterLock rl(m_mutex);
	return m_pimpl->GetFileSize();
};

blksize_t FileID::GetBlockSize() const noexcept
{
	WriterLock wl(m_mutex);
	return m_pimpl->GetBlockSize();
};

const dev_ino_pair FileID::GetUniqueFileIdentifier() const noexcept
{
	WriterLock wl(m_mutex);
	return m_pimpl->GetUniqueFileIdentifier();
}

void FileID::SetFileDescriptorMode(FileAccessMode fam, FileCreationFlag fcf)
{
	int temp_flags = fam | fcf;

	{
		WriterLock wl(m_mutex);
		m_pimpl->m_open_flags = temp_flags;
	}
}

FileDescriptor FileID::GetFileDescriptor()
{
	WriterLock wl(m_mutex);
	return m_pimpl->GetFileDescriptor();
}

dev_t FileID::GetDev() const noexcept
{
	WriterLock wl(m_mutex);
	return m_pimpl->GetDev();
};

void FileID::SetDevIno(dev_t d, ino_t i) noexcept
{
	WriterLock wl(m_mutex);
	m_pimpl->SetDevIno(d, i);
}

void FileID::SetStatInfo(const struct stat &stat_buf) noexcept
{
	WriterLock wl(m_mutex);
	m_pimpl->SetStatInfo(stat_buf);
}


