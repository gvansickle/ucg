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


#include <future/memory.hpp> // For std::make_unique<>
#include <libext/string.hpp>

#include "FileID.h"

#include <fcntl.h> // For AT_FDCWD, AT_NO_AUTOMOUNT
#include <unistd.h> // For close().
#include <sys/stat.h>

#include "double_checked_lock.hpp"


//////////////////////////////////
///


FileID::impl::impl(std::shared_ptr<FileID> at_dir_fileid, std::string bname)
	: m_at_dir(at_dir_fileid), m_basename(bname)
{
	/// @note Taking basename by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.

	LOG(DEBUG) << "2-param const., m_basename=" << m_basename << ", m_at_dir=" << (!!m_at_dir ? (m_at_dir->m_pimpl->m_path) : "<nullptr>");

	if(is_pathname_absolute(bname))
	{
		// Save an expensive recursive call to m_at_dir->GetPath() in the future.
		m_path = bname;
	}
}

FileID::impl::impl(std::shared_ptr<FileID> at_dir_fileid, std::string bname, std::string pname,
		const struct stat *stat_buf, FileType type)
		: m_at_dir(at_dir_fileid), m_basename(bname), m_path(pname), m_file_type(type)
{
	LOG(DEBUG) << "5-param const., m_basename=" << m_basename << ", m_at_dir=" << (!!m_at_dir ? (m_at_dir->m_pimpl->m_path) : "<nullptr>");
	if(stat_buf != nullptr)
	{
		SetStatInfo(*stat_buf);
	}
}

const std::string& FileID::impl::GetBasename() const noexcept
{
	return m_basename;
};

// Stats
std::atomic<std::uint64_t> FileID::impl::m_atomic_fd_max_reg;
std::atomic<std::uint64_t> FileID::impl::m_atomic_fd_max_dir;
std::atomic<std::uint64_t> FileID::impl::m_atomic_fd_max_other;

const FileDescriptor* FileID::impl::GetFileDescriptor()
{
	if(m_file_descriptor.empty())
	{
		// File hasn't been opened.

		if(m_open_flags == 0)
		{
			throw std::runtime_error("m_open_flags is not set");
		}

#if 0
		// Maintain stats on the number of openat()s we do for each of FT_REG/FT_DIR.
		switch(m_file_type)
		{
		case FT_REG:
		{
			com_exch_loop(m_atomic_fd_max_reg, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		case FT_DIR:
		{
			com_exch_loop(m_atomic_fd_max_dir, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		default:
		{
			com_exch_loop(m_atomic_fd_max_other, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		}
#endif

		if(m_at_dir)
		{
			int atdirfd = m_at_dir->GetFileDescriptor().GetFD();
			int tempfd = openat(atdirfd, GetBasename().c_str(), m_open_flags);
			if(tempfd == -1)
			{
				//LOG(DEBUG) << "OPENAT FAIL: " << explain_openat(atdirfd, GetBasename().c_str(), m_open_flags, 0666);
				throw FileException("GetFileDescriptor(): openat(" + GetBasename() + ") with valid m_at_dir=" + std::to_string(atdirfd) + " failed");
			}
			m_file_descriptor = make_shared_fd(tempfd);
		}
	}

	return &m_file_descriptor;
}

void FileID::impl::SetDevIno(dev_t d, ino_t i) noexcept
{
	m_dev = d;
	m_unique_file_identifier = dev_ino_pair(d, i);
}

int FileID::impl::TryGetFD() const noexcept
{
	if(!m_file_descriptor.empty())
	{
		return m_file_descriptor.GetFD();
	}

	// No descriptor open yet.
	return -1;
}

void* FileID::impl::LazyLoadStatInfo() const noexcept
{
	if(m_stat_info_valid)
	{
		// Already set.
		return (void*)1;
	}

	// We don't have stat info and now we need it.
	// Get it from the filename.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate" // Quiet some warnings that this will terminate the program, that's intentional.
	if(!m_at_dir)
	{
		throw std::runtime_error("should always have an at-dir");
	}
#pragma GCC diagnostic pop

	struct stat stat_buf;
	bool fstat_success = m_at_dir->FStatAt(m_basename, &stat_buf, AT_NO_AUTOMOUNT);

	if(fstat_success)
	{
		SetStatInfo(stat_buf);
	}
	else
	{
		m_file_type = FT_STAT_FAILED;
	}

	return (void*)1;
}

void FileID::impl::SetStatInfo(const struct stat &stat_buf) const noexcept
{
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

	m_stat_info_valid = true;
}

const std::string& FileID::impl::ResolvePath() const
{
	// Do we not have a full path already?
	if(m_path.empty())
	{
		// No.  Build the full path.
		auto at_path = m_at_dir->GetPath();
		if(at_path != ".")
		{
			m_path.reserve(at_path.length() + m_basename.size()+2);
			m_path.append(at_path);
			m_path.append("/", 1);
			m_path.append(m_basename);
		}
		else
		{
			m_path = m_basename;
		}
	}

	return m_path;
}

/// //////////////////////////////



// Default constructor.
// Note that it's defined here in the cpp vs. in the header because it needs to be able to see the full definition of UnsynchronizedFileID.
FileID::FileID() //: FileID(path_known_cwd)
{
	LOG(DEBUG) << "Default constructor called";
}

// Copy constructor.
FileID::FileID(const FileID& other)
	: m_pimpl((ReaderLock(other.m_mutex), std::make_unique<FileID::impl>(*other.m_pimpl)))
{
	LOG(DEBUG) << "Copy constructor called";
	if(!m_pimpl)
	{
		throw std::runtime_error("no pimpl on copy");
	}
	LOG(DEBUG) << "Copy constructor end";
};

// Move constructor.
FileID::FileID(FileID&& other)
	: m_pimpl(std::move((WriterLock(other.m_mutex), other.m_pimpl)))
{
	LOG(DEBUG) << "Move constructor called";
	if(!m_pimpl)
	{
		throw std::runtime_error("no pimpl on move");
	}
	LOG(DEBUG) << "Move constructor end";
};

FileID::FileID(path_known_cwd_tag)
	: m_pimpl(std::make_unique<FileID::impl>(nullptr, ".", ".", nullptr, FT_DIR))
{
	// Open the file descriptor immediately, since we don't want to use openat() here unlike in all other cases.
	SetFileDescriptorMode(FAM_SEARCH, FCF_DIRECTORY | FCF_NOCTTY);
	int tempfd = open(".", O_SEARCH | O_DIRECTORY | O_NOCTTY);
	if(tempfd == -1)
	{
		LOG(DEBUG) << "Error in fdcwd constructor: " << LOG_STRERROR();
	}
	m_pimpl->m_file_descriptor = make_shared_fd(tempfd);

	LOG(DEBUG) << "FDCWD constructor, file descriptor: " << m_pimpl->m_file_descriptor.GetFD();

}

FileID::FileID(path_known_relative_tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, const struct stat *stat_buf, FileType type)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, basename, "", stat_buf, type))
{
	// basename is a file relative to at_dir_fileid, and we also have stat info for it.
	if(stat_buf != nullptr)
	{
		m_stat_info_witness.store((void*)1);
	}
}

FileID::FileID(path_known_relative_tag, std::shared_ptr<FileID> at_dir_fileid, std::string basename, FileType type)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, basename, "", nullptr, type))
{
	/// @note Taking basename by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.
}

FileID::FileID(path_known_absolute_tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, pathname /*==basename*/, pathname, nullptr, type))
{
	/// @note Taking pathname by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.

	if(is_pathname_absolute(pathname))
	{
		m_path_witness.store(&m_pimpl->m_path);
	}
}

FileID::FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileAccessMode fam, FileCreationFlag fcf)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, pathname))
{
	SetFileDescriptorMode(fam, fcf);
}

FileID& FileID::operator=(const FileID& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		ReaderLock other_lock(other.m_mutex, std::defer_lock);
		std::lock(this_lock, other_lock);
		LOG(DEBUG) << "COPY ASSIGN";
		m_pimpl = std::make_unique<FileID::impl>(*other.m_pimpl);

		m_file_descriptor_witness = other.m_file_descriptor_witness.load();
		m_stat_info_witness = other.m_stat_info_witness.load();
		m_path_witness = other.m_path_witness.load();
	}
	return *this;
};

FileID& FileID::operator=(FileID&& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		WriterLock other_lock(other.m_mutex, std::defer_lock);
		LOG(DEBUG) << "MOVE ASSIGN";
		std::lock(this_lock, other_lock);

		m_pimpl = std::move(other.m_pimpl);
		m_file_descriptor_witness = other.m_file_descriptor_witness.load();
		m_stat_info_witness = other.m_stat_info_witness.load();
		m_path_witness = other.m_path_witness.load();
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
	// This is const after construction, and always exists, so it doesn't need a lock here.
	return m_pimpl->GetBasename();
};

const std::string& FileID::GetPath() const noexcept
{
	return *DoubleCheckedLock<std::string*>(m_path_witness, m_mutex, [this](){ return (std::string*)&(m_pimpl->ResolvePath()); });
}

FileType FileID::GetFileType() const noexcept
{
	/// @todo Not reliant on stat, could be optimized.
	DoubleCheckedLock<void*>(m_stat_info_witness, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_file_type;
}

off_t FileID::GetFileSize() const noexcept
{
	DoubleCheckedLock<void*>(m_stat_info_witness, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_size;
};

blksize_t FileID::GetBlockSize() const noexcept
{
	DoubleCheckedLock<void*>(m_stat_info_witness, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_block_size;
};

const dev_ino_pair FileID::GetUniqueFileIdentifier() const noexcept
{
	/// @todo This is not necessarily dependent on stat info, could be optimized.

	DoubleCheckedLock<void*>(m_stat_info_witness, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo();});
	return m_pimpl->m_unique_file_identifier;
}

void FileID::SetFileDescriptorMode(FileAccessMode fam, FileCreationFlag fcf)
{
	int temp_flags = fam | fcf;

	LOG(DEBUG) << "Setting file descriptor mode flags: " << to_string(temp_flags, std::hex);

	{
		WriterLock wl(m_mutex);
		m_pimpl->m_open_flags = temp_flags;
	}
}

bool FileID::FStatAt(const std::string &name, struct stat *statbuf, int flags)
{
	int retval = fstatat(GetFileDescriptor().GetFD(), name.c_str(), statbuf, flags);

	if(retval == -1)
	{
		WARN() << "Attempt to stat file '" << name << "' in directory '" << GetPath() << "' failed: " << LOG_STRERROR();
		// Note: We don't clear errno here, we want to be able to look at it in the caller.
		//errno = 0;
		return false;
	}

	return true;
}

#if 0
FileID FileID::OpenAt(const std::string &name, FileType type, int flags)
{
	FileID retval;

	/// @todo
	return retval;
}
#endif

DIR *FileID::OpenDir()
{
	int fd = m_pimpl->TryGetFD();
	int dirfd {0};
	if(fd < 0)
	{
		dirfd = open(GetPath().c_str(), O_RDONLY | O_NOCTTY | O_DIRECTORY);
	}
	else
	{
		// We already had a file descriptor.  Dup it, because fdopendir() takes ownership of it.
		dirfd = dup(fd);
	}
	return fdopendir(dirfd);
}

void FileID::CloseDir(DIR *d)
{
	closedir(d);
}

const FileDescriptor& FileID::GetFileDescriptor()
{
	return *DoubleCheckedLock<FileDescriptor*>(m_file_descriptor_witness, m_mutex, [this](){ return m_pimpl->GetFileDescriptor();});
}

dev_t FileID::GetDev() const noexcept
{
	//WriterLock wl(m_mutex);
	//return m_pimpl->GetDev();
	DoubleCheckedLock<void*>(m_stat_info_witness, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });

	return m_pimpl->m_dev;
}

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


std::ostream& operator<<(std::ostream &ostrm, const FileID &fileid)
{
	fileid.m_pimpl->dump_stats(ostrm, *fileid.m_pimpl);
	return ostrm;
}

