/*
 * Copyright 2016-2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "DoubleCheckedLock.hpp"

#define M_ENABLE_FD_STATS 0

/**
 * pImpl factorization of the FileID class.  This is the unsynchronized "pImpl" part which holds all the data.
 * It does not concern itself with concurrency issues with respect to initialization, copying, moving, or assigning.
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
	impl(std::shared_ptr<FileID> at_dir_fileid, std::string base_or_pathname);
	impl(std::shared_ptr<FileID> at_dir_fileid, std::string basename, std::string pathname,
			const struct stat *stat_buf = nullptr, FileType type = FT_UNINITIALIZED);
	///@}

	/// Destructor.
	~impl()
	{
		if(m_file_descriptor >= 0)
		{
			close(m_file_descriptor);
		}
	}

	const std::string& GetBasename() const noexcept;

	/// Resolve and cache this FileID's path.  Recursively visits its parent directories to do so.
	const std::string& ResolvePath() const;

	FileID::IsValid GetFileDescriptor();

//protected:
	std::ostream& dump_stats(std::ostream &ostrm, const FileID::impl &impl)
	{
		return ostrm << "Max descriptors, regular: " << impl.m_atomic_fd_max_reg << "\n"
				<< "Max descriptors, dir: " << impl.m_atomic_fd_max_dir << "\n"
				<< "Max descriptors, other: " << impl.m_atomic_fd_max_other << "\n";
	}

	/**
	 * If we have a file descriptor already, return it.  Else return -1.
	 * @return
	 */
	int TryGetFD() const noexcept;

//private:

	FileID::IsValid LazyLoadStatInfo() const noexcept;

	void SetStatInfo(const struct stat &stat_buf) const noexcept;

	void SetDevIno(dev_t d, ino_t i) noexcept;

// Data members.

	/// Shared pointer to the directory this FileID is in.
	/// The constructors ensure that this member always exists and is valid.
	std::shared_ptr<FileID> m_at_dir;

	/// The basename of this file.
	/// We define this somewhat differently here: This is either:
	/// - The full absolute path, or
	/// - The path relative to m_at_dir, which may consist of more than one path element.
	/// In any case, it is always equal to the string passed into the constructor, and this will always exist and is valid.
	std::string m_basename;

	/// The full m_at_dir-relative path to this file.
	/// This will be lazily evaluated when needed, unless an absolute path is passed in to the constructor.
	mutable std::string m_path;

	/// Flags to use when we open the file descriptor.
	mutable int m_open_flags { 0 };

#if 0
	/// The file descriptor object.
	mutable FileDescriptor m_file_descriptor {};
#endif
	mutable int m_file_descriptor = -987;

	/// @name Info normally gathered from a stat() call.
	///@{
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

	// Stats
	static std::atomic<std::uint64_t> m_atomic_fd_max_reg;
	static std::atomic<std::uint64_t> m_atomic_fd_max_dir;
	static std::atomic<std::uint64_t> m_atomic_fd_max_other;
};

/// @name Compile-time invariants for the FileID::impl class.
/// @{
static_assert(std::is_assignable<FileID::impl, FileID::impl>::value, "FileID::impl must be assignable to itself.");
static_assert(std::is_copy_assignable<FileID::impl>::value, "FileID::impl must be copy assignable to itself.");
static_assert(std::is_move_assignable<FileID::impl>::value, "FileID::impl must be move assignable to itself.");
/// @}


FileID::impl::impl(std::shared_ptr<FileID> at_dir_fileid, std::string bname)
	: m_at_dir(std::move(at_dir_fileid)), m_basename(bname)
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
		: m_at_dir(std::move(at_dir_fileid)), m_basename(std::move(bname)), m_path(std::move(pname)), m_file_type(type)
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

FileID::IsValid FileID::impl::GetFileDescriptor()
{
	if(m_file_descriptor < 0)
	{
		// File hasn't been opened.

		if(m_open_flags == 0)
		{
			throw std::runtime_error("m_open_flags is not set");
		}

#if M_ENABLE_FD_STATS
		// Maintain stats on the number of openat()s we do for each of FT_REG/FT_DIR.
		switch(m_file_type)
		{
		case FT_REG:
		{
			comp_exch_loop(m_atomic_fd_max_reg, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		case FT_DIR:
		{
			comp_exch_loop(m_atomic_fd_max_dir, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		default:
		{
			comp_exch_loop(m_atomic_fd_max_other, [](uint64_t old_val){ return old_val + 1; });
			break;
		}
		}
#endif

		if(m_at_dir)
		{
			int tempfd = -1;
			if(m_file_type == FT_REG)
			{
				int atdirfd = m_at_dir->GetFileDescriptor();
				tempfd = openat(atdirfd, GetBasename().c_str(), m_open_flags);
				if(tempfd == -1)
				{
					throw FileException("GetFileDescriptor(): openat(" + GetBasename() + ") with valid m_at_dir=" + std::to_string(atdirfd) + " failed");
				}
			}
			else if(m_file_type == FT_DIR)
			{
				// Should only get here via a recursive call from the FT_REG case, for an at_dir.
				tempfd = open(m_path.c_str(), m_open_flags | O_PATH);
				if(tempfd == -1)
				{
					throw FileException("GetFileDescriptor(): open(" + m_path + ") failed");
				}
			}

			m_file_descriptor = tempfd;
		}
	}

	return FileID::FILE_DESC;
}

void FileID::impl::SetDevIno(dev_t d, ino_t i) noexcept
{
	m_dev = d;
	m_unique_file_identifier = dev_ino_pair(d, i);
}

int FileID::impl::TryGetFD() const noexcept
{
	if(m_file_descriptor >= 0)
	{
		return m_file_descriptor;
	}

	// No descriptor open yet.
	return -1;
}

FileID::IsValid FileID::impl::LazyLoadStatInfo() const noexcept
{
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
	bool fstat_success = false;

	if(m_file_descriptor >= 0)
	{
		// We have a file descriptor, stat it directly.
		int status = fstat(m_file_descriptor, &stat_buf);
		fstat_success = (status == 0);
	}
	else
	{
		fstat_success = m_at_dir->FStatAt(m_basename, &stat_buf, AT_NO_AUTOMOUNT);
	}

	if(fstat_success)
	{
		SetStatInfo(stat_buf);
	}
	else
	{
		m_file_type = FT_STAT_FAILED;
	}

	return FileID::UUID | FileID::STATINFO | FileID::TYPE;
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

/////////////////////////////////
/// END OF FILEID::IMPL.
/////////////////////////////////
/// BEGINNING OF FILEID.
/////////////////////////////////


// Default constructor.
// Note that it's defined here in the cpp vs. in the header because it needs to be able to see the full definition of FileID::impl.
FileID::FileID()
{
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
	m_pimpl->m_file_descriptor = tempfd;

	LOG(DEBUG) << "FDCWD constructor, file descriptor: " << m_pimpl->m_file_descriptor;

	m_valid_bits.fetch_or(FILE_DESC | TYPE | PATH);

}

FileID::FileID(path_known_relative_tag, const std::shared_ptr<FileID>& at_dir_fileid, const std::string &bname,
		const struct stat *stat_buf,
		FileType type,
		dev_t d, ino_t i,
		FileAccessMode fam, FileCreationFlag fcf)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, bname, "", stat_buf, type))
{
	uint_fast8_t orbits = NONE;

	// basename is a file relative to at_dir_fileid.
	if(stat_buf != nullptr)
	{
		// Bonus: we already have stat info for this file.
		orbits |= UUID | STATINFO | TYPE;
	}
	else if(type != FT_UNINITIALIZED)
	{
		orbits |= TYPE;
	}

	if(d != static_cast<dev_t>(-1) && i != 0)
	{
		m_pimpl->SetDevIno(d, i);
		orbits |= UUID;
	}

	if(fam != FAM_UNINITIALIZED && fcf != FCF_UNINITIALIZED)
	{
		m_pimpl->m_open_flags = fam | fcf;
	}

	m_valid_bits.fetch_or(orbits);
}

FileID::FileID(path_known_absolute_tag, std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileType type)
	: m_pimpl(std::make_unique<FileID::impl>(at_dir_fileid, pathname /*==basename*/, pathname, nullptr, type))
{
	/// @note Taking pathname by value since we are always storing it.
	/// Full openat() semantics:
	/// - If pathname is absolute, at_dir_fd is ignored.
	/// - If pathname is relative, it's relative to at_dir_fd.

	uint_fast8_t orbits = NONE;

	if(is_pathname_absolute(pathname))
	{
		orbits |= PATH;
	}
	if(type != FT_UNINITIALIZED)
	{
		orbits |= TYPE;
	}
	m_valid_bits.fetch_or(orbits);
}

FileID::FileID(std::shared_ptr<FileID> at_dir_fileid, std::string pathname, FileAccessMode fam, FileCreationFlag fcf)
	: m_pimpl(std::make_unique<FileID::impl>(std::move(at_dir_fileid), pathname))
{
	SetFileDescriptorMode(fam, fcf);

	uint_fast8_t orbits = NONE;

	/// @todo This check is kind of out of place.  It ends up being done twice, here and in the impl constructor.
	if(is_pathname_absolute(pathname))
	{
		orbits |= PATH;
	}

	m_valid_bits.fetch_or(orbits);
}

FileID& FileID::operator=(const FileID& other)
{
	if(this != &other)
	{
		WriterLock this_lock(m_mutex, std::defer_lock);
		ReaderLock other_lock(other.m_mutex, std::defer_lock);
		std::lock(this_lock, other_lock);
		m_pimpl = std::make_unique<FileID::impl>(*other.m_pimpl);

		m_valid_bits = other.m_valid_bits.load();
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
		m_valid_bits = other.m_valid_bits.load();
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
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, PATH, m_mutex, [this](){ m_pimpl->ResolvePath(); return PATH; });
	return m_pimpl->m_path;
}

FileType FileID::GetFileType() const noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, TYPE, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_file_type;
}

off_t FileID::GetFileSize() const noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, STATINFO, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_size;
};

blksize_t FileID::GetBlockSize() const noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, STATINFO, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_block_size;
};

const dev_ino_pair FileID::GetUniqueFileIdentifier() const noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, UUID, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo();});
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
	int atdir_fd = GetFileDescriptor();
	int retval = fstatat(atdir_fd, name.c_str(), statbuf, flags);

	if(retval == -1)
	{
		WARN() << "Attempt to stat file '" << name << "' in directory '" << GetPath() << "' (atfd=" << atdir_fd << ") failed: " << LOG_STRERROR();
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
	int fd_dir {0};
	if(fd < 0)
	{
		fd_dir = open(GetPath().c_str(), O_RDONLY | O_NOATIME | O_NOCTTY | O_DIRECTORY);
	}
	else
	{
		// We already have a file descriptor.  Dup it, because fdopendir() takes ownership of it.
		fd_dir = dup(fd);
	}
	return fdopendir(fd_dir);
}

void FileID::CloseDir(DIR *d)
{
	closedir(d);
}

int FileID::GetFileDescriptor()
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, FILE_DESC, m_mutex, [this](){ return m_pimpl->GetFileDescriptor();});
	return m_pimpl->m_file_descriptor;
}

dev_t FileID::GetDev() const noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, UUID, m_mutex, [this](){ return m_pimpl->LazyLoadStatInfo(); });
	return m_pimpl->m_dev;
}

void FileID::SetDevIno(dev_t d, ino_t i) noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, UUID, m_mutex,
			[&](){ m_pimpl->SetDevIno(d, i); return UUID; });
}

void FileID::SetStatInfo(const struct stat &stat_buf) noexcept
{
	DoubleCheckedMultiLock<uint8_t>(m_valid_bits, STATINFO, m_mutex,
			[&](){ m_pimpl->SetStatInfo(stat_buf); return UUID | STATINFO | TYPE; });
}


std::ostream& operator<<(std::ostream &ostrm, const FileID &fileid)
{
	fileid.m_pimpl->dump_stats(ostrm, *fileid.m_pimpl);
	return ostrm;
}

