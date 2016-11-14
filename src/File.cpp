/*
 * Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "File.h"

#include <iostream>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Logger.h"

File::File(FileID&& file_id, std::shared_ptr<ResizableArray<char>> storage) : m_storage(storage)
{
	m_fileid = std::move(file_id);
	m_fileid.SetFileDescriptorMode(FAM_RDONLY, FCF_NOATIME | FCF_NOCTTY);

	int file_descriptor { -1 };

	try
	{
		file_descriptor = m_fileid.GetFileDescriptor().GetFD();

		/// @todo Shouldn't need this anymore.
		if(file_descriptor == -1)
		{
			// Couldn't open the file, throw exception.
			LOG(DEBUG) << "bad file descriptor: fd=" << file_descriptor;
			throw std::system_error(errno, std::generic_category());
		}
	}
	catch(const std::system_error &e)
	{
		// Rethrow.
		throw;
	}
	catch(...)
	{
		// Rethrow anything else.
		throw;
	}

	ssize_t file_size = m_fileid.GetFileSize();
	LOG(INFO) << "... file size is: " << file_size;
	LOG(INFO) << "... file type is: " << m_fileid.GetFileType();

	// If filesize is 0, skip.
	if(file_size == 0)
	{
		//close(m_file_descriptor);
		//m_file_descriptor = -1;
		return;
	}

	// Read or mmap the file into memory.
	// Note that this closes the file descriptor.
	// Note: per info here:
	// http://stackoverflow.com/questions/34498825/io-blksize-seems-just-return-io-bufsize
	// https://github.com/coreutils/coreutils/blob/master/src/ioblksize.h#L23-L57
	// http://unix.stackexchange.com/questions/245499/how-does-cat-know-the-optimum-block-size-to-use
	// ...it seems that as of ~2014, experiments show the minimum I/O size should be >=128KB.
	// *stat() seems to return 4096 in all my experiments so far, so we'll clamp it to a min of 128KB and a max of
	// something not unreasonable, e.g. 1M.
	auto io_size = clamp(m_fileid.GetBlockSize(), static_cast<blksize_t>(0x20000), static_cast<blksize_t>(0x100000));
	m_file_data = GetFileData(file_descriptor, file_size, io_size);

	///m_file_descriptor = -1;

	if(m_file_data == MAP_FAILED)
	{
		// Mapping failed.
		ERROR() << "Couldn't map file '" << m_fileid.GetPath() << "'";
		throw std::system_error(errno, std::system_category());
	}
}


File::File(const std::string &filename, std::shared_ptr<ResizableArray<char>> storage)
	: File(FileID(std::make_shared<FileID>(FileID::path_known_cwd_tag()), filename), storage)
{
#if 0 /// @todo DELETE
	// Save the filename.
	m_filename = filename;

	// open() the file.  We have to do this regardless of whether we'll subsequently mmap() or read().
	m_file_descriptor = open(filename.c_str(), O_RDONLY);

	if(m_file_descriptor == -1)
	{
		// Couldn't open the file, throw exception.
		int temp_errno = errno;
		errno = 0;
		throw std::system_error(temp_errno, std::generic_category());
	}

	// Check the file size.
	struct stat st;
	int retval = fstat(m_file_descriptor, &st);
	if(retval != 0)
	{
		// fstat() failed, throw an exception.
		close(m_file_descriptor);
		m_file_descriptor = -1;
		int temp_errno = errno;
		errno = 0;
		throw std::system_error(temp_errno, std::generic_category());
	}

	// Make sure this is a regular file.
	if(!S_ISREG(st.st_mode))
	{
		// Not a regular file, we shouldn't have been called.
		close(m_file_descriptor);
		m_file_descriptor = -1;
		throw FileException("File is not regular file: \"" + filename + "\"");
	}

	m_file_size = st.st_size;
	// If filesize is 0, skip.
	if(m_file_size == 0)
	{
		close(m_file_descriptor);
		m_file_descriptor = -1;
		return;
	}

	// Read or mmap the file into memory.
	// Note that this closes the file descriptor.
	m_file_data = GetFileData(m_file_descriptor, m_file_size, 4096);
	m_file_descriptor = -1;

	if(m_file_data == MAP_FAILED)
	{
		// Mapping failed.
		ERROR() << "Couldn't map file \"" << filename << "\"";
		throw std::system_error(errno, std::system_category());
	}
#endif
}

File::~File()
{
	// Clean up.
	FreeFileData(m_file_data, m_fileid.GetFileSize());
}

const char* File::GetFileData(int file_descriptor, size_t file_size, size_t preferred_block_size)
{
	const char *file_data = static_cast<const char *>(MAP_FAILED);

	if(false) /// @todo This is very broken right now.  (m_use_mmap)
	{
		file_data = static_cast<const char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE /*| MAP_POPULATE*/, file_descriptor, 0));

		if(file_data == MAP_FAILED)
		{
			// Mapping failed.
			close(file_descriptor);
			return file_data;
		}

		// Hint that we'll be sequentially reading the mmapped file soon.
		posix_madvise(const_cast<char*>(file_data), file_size, POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED);
	}
	else
	{
		// Not using mmap().

#ifdef HAVE_POSIX_FADVISE // OSX doesn't have it.
		// Notify the filesystem of our intentions to:
		// - Access the file sequentially.  This will cause Linux to double the file's readahead window.
		// - That we'll need the contents in the near future.  Per the Linux manpage, this will cause it to start
		//   a non-blocking read of the file.
		// Explicitly ignoring the return value for Coverity's sake.  If the advice is ignored, we should still be functional.
		(void)posix_fadvise(file_descriptor, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED);
#endif

		m_storage->reserve_no_copy(file_size, preferred_block_size);
		file_data = m_storage->data();

		// Read in the whole file.
		ssize_t retval = 0;
		while((retval = read(file_descriptor, const_cast<char*>(file_data), file_size)) > 0);
		if(retval < 0)
		{
			// read error.
			ERROR() << "read() error on file '" << "<<TODO>>" << "', descriptor " << file_descriptor << ": " << LOG_STRERROR();
			errno = 0;
		}
	}

	// We don't need the file descriptor anymore.
	///@todo close(file_descriptor);

	return file_data;
}

void File::FreeFileData(const char* file_data, size_t file_size) noexcept
{
	if(m_use_mmap)
	{
		munmap(const_cast<char*>(file_data), file_size);
	}
}
