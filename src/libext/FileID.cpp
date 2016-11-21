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

#include <atomic>

//using cache_filler_type = std::function<ReturnType() noexcept>;
/**
 *
 * @param wrap          An instance of ats::atomic<ReturnType>.
 * @param mutex
 * @param cache_filler
 * @return
 */
template < typename ReturnType, typename AtomicTypeWrapper = std::atomic<ReturnType>, typename CacheFillerType = std::function<ReturnType()>&, typename MutexType = std::mutex >
ReturnType DoubleCheckedLock(AtomicTypeWrapper &wrap, MutexType &mutex, CacheFillerType cache_filler)
{
#if 1
	ReturnType temp_retval = wrap.load(std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_acquire);  	// Guaranteed to observe everything done in another thread before
			 	 	 	 	 	 	 	 	 	 	 	 	// the std::atomic_thread_fence(std::memory_order_release) below.
	if(temp_retval == nullptr)
	{
		// First check says we don't have the cached value yet.
		std::unique_lock<MutexType> lock(mutex);
		// One more try.
		temp_retval = wrap.load(std::memory_order_relaxed);
		if(temp_retval == nullptr)
		{
			// Still no cached value.  We'll have to do the heavy lifting.
			temp_retval = const_cast<ReturnType>(cache_filler());
			std::atomic_thread_fence(std::memory_order_release);
			wrap.store(temp_retval, std::memory_order_relaxed);
		}
	}
#else
#endif
	return temp_retval;
}


template <typename T, typename Lambda = std::function<T(T&)>&>
void com_exch_loop(std::atomic<T> &atomic_var, Lambda val_changer)
{
	T old_val;
	while(!atomic_var.compare_exchange_weak(old_val, val_changer(old_val))) {};
}


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
		else
		{
			// We should only get here if we are the root of the traversal.
			int tempfd = openat(AT_FDCWD, GetBasename().c_str(), m_open_flags);
			if(tempfd == -1)
			{
				//LOG(DEBUG) << "OPENAT FAIL: " << explain_openat(AT_FDCWD, GetBasename().c_str(), m_open_flags, 0666);
				throw FileException("GetFileDescriptor(): openat(" + GetBasename() + ") with invalid m_at_dir=" + std::to_string(AT_FDCWD) + " failed");
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

void FileID::impl::LazyLoadStatInfo() const noexcept
{
	if(m_stat_info_valid)
	{
		// Already set.
		return;
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
		m_path.reserve(64 + m_basename.size()); // Random number.
		auto at_path = m_at_dir->GetPath();
		if(at_path != ".")
		{
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
}

FileID::FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileAccessMode fam, FileCreationFlag fcf)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, pathname))
{
	SetFileDescriptorMode(fam, fcf);
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

FileID& FileID::operator=(const FileID& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		ReaderLock other_lock(other.m_mutex, std::defer_lock);
		/// @todo
#if 0
		std::unique_lock<std::mutex> this_l2(m_the_mutex, std::defer_lock);
		std::unique_lock<std::mutex> other_l2(other.m_the_mutex, std::defer_lock);
#endif
		std::lock(this_lock, other_lock);
		LOG(DEBUG) << "COPY ASSIGN";
		//m_pimpl = std::make_unique<FileID::impl>(*other.m_pimpl);
		m_pimpl.reset(new impl(*other.m_pimpl));
	}
	return *this;
};

FileID& FileID::operator=(FileID&& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		WriterLock other_lock(other.m_mutex, std::defer_lock);
#if 0
		/// @todo
		std::unique_lock<std::mutex> this_l2(m_the_mutex, std::defer_lock);
		std::unique_lock<std::mutex> other_l2(other.m_the_mutex, std::defer_lock);
#endif
		LOG(DEBUG) << "MOVE ASSIGN";
		std::lock(this_lock, other_lock);

		m_pimpl = std::move(other.m_pimpl);
		m_file_descriptor_witness = other.m_file_descriptor_witness.load();
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

const std::string& FileID::GetPath() const noexcept
{
#if 1
	{
		ReaderLock rl(m_mutex);

		if(!m_pimpl->m_path.empty())
		{
			// m_path has already been lazily evaluated and is available in a std::string.
			return m_pimpl->m_path;
		}
	}
	// Else, we need to get a write lock, and evaluate the path.

	WriterLock wl(m_mutex);
	m_pimpl->ResolvePath();

	return m_pimpl->m_path;
#else
	auto path_filler = [this](){ return (std::string*)&m_pimpl->ResolvePath(); };
	//std::function<const std::string& (FileID::impl&)> path_filler = &impl::ResolvePath;
	return *DoubleCheckedLock<std::string*>(m_atomic_path_ptr, m_the_mutex, path_filler);
#endif
}

std::shared_ptr<FileID> FileID::GetAtDir() const noexcept
{
	// Only need reader lock here, m_pimpl->m_at_dir will always exist.
	ReaderLock rl(m_mutex);
	return m_pimpl->GetAtDir();
};

FileType FileID::GetFileType() const noexcept
{
	WriterLock rl(m_mutex);

	return m_pimpl->GetFileType();
}

off_t FileID::GetFileSize() const noexcept
{
	{
		ReaderLock rl(m_mutex);
		if(m_pimpl->m_stat_info_valid)
		{
			return m_pimpl->m_size;
		}
	}
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

	LOG(DEBUG) << "Setting file descriptor mode flags: " << to_string(temp_flags, std::hex);

	{
		WriterLock wl(m_mutex);
		m_pimpl->m_open_flags = temp_flags;
	}
}

void FileID::FStatAt(const std::string &name, struct stat *statbuf, int flags)
{
	int retval = fstatat(GetFileDescriptor().GetFD(), name.c_str(), statbuf, flags);
	if(retval == -1)
	{
		WARN() << "Attempt to stat file '" << name << "' in directory '" << GetPath() << "' failed: " << LOG_STRERROR();
		errno = 0;
		return;
	}
}

const FileDescriptor& FileID::GetFileDescriptor()
{
#if 0
	{
		ReaderLock rl(m_mutex);
		if(!m_pimpl->m_file_descriptor.empty())
		{
			return m_pimpl->m_file_descriptor;
		}
	}
	WriterLock wl(m_mutex);
	return m_pimpl->GetFileDescriptor();
#endif
	return *DoubleCheckedLock<FileDescriptor*>(m_file_descriptor_witness, m_mutex, [this](){ return m_pimpl->GetFileDescriptor();});
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


std::ostream& operator<<(std::ostream &ostrm, const FileID &fileid)
{
	fileid.m_pimpl->dump_stats(ostrm, *fileid.m_pimpl);
	return ostrm;
}

