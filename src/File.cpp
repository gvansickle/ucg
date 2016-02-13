/*
 * Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of UltimateCodeSearch.
 *
 * UltimateCodeSearch is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * UltimateCodeSearch is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * UltimateCodeSearch.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file */

#include "File.h"

#include <iostream>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

File::File(const std::string &filename)
{
	// open() the file.  We have to do this regardless of whether we'll subsequently mmap() or read().
	m_file_descriptor = open(filename.c_str(), O_RDONLY);

	if(m_file_descriptor == -1)
	{
		// Couldn't open the file, throw exception.
		throw std::system_error(errno, std::generic_category());
	}

	// Check the file size.
	struct stat st;
	int retval = fstat(m_file_descriptor, &st);
	if(retval != 0)
	{
		// fstat() failed, throw an exception.
		close(m_file_descriptor);
		m_file_descriptor = -1;
		throw std::system_error(errno, std::generic_category());
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
		//std::cerr << "WARNING: Filesize of \"" << filename << "\" is 0" << std::endl;
		close(m_file_descriptor);
		m_file_descriptor = -1;
		/// @todo Throw here?
		return;
	}

	// Read or mmap the file into memory.
	// Note that this closes the file descriptor.
	m_file_data = GetFileData(m_file_descriptor, m_file_size);
	m_file_descriptor = -1;

	if(m_file_data == MAP_FAILED)
	{
		// Mapping failed.
		std::cerr << "ERROR: Couldn't map file \"" << filename << "\"" << std::endl;
		throw std::system_error(errno, std::system_category());
	}
}

File::~File()
{
	// Clean up.
	FreeFileData(m_file_data, m_file_size);
}

const char* File::GetFileData(int file_descriptor, size_t file_size)
{
	const char *file_data = static_cast<const char *>(MAP_FAILED);

	if(m_use_mmap)
	{
		file_data = static_cast<const char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, file_descriptor, 0));

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
		file_data = new char [file_size];

		// Read in the whole file.
		while(read(file_descriptor, const_cast<char*>(file_data), file_size) > 0);
	}

	// We don't need the file descriptor anymore.
	close(file_descriptor);

	return file_data;
}

void File::FreeFileData(const char* file_data, size_t file_size)
{
	if(m_use_mmap)
	{
		munmap(const_cast<char*>(file_data), file_size);
	}
	else
	{
		delete [] file_data;
	}
}
