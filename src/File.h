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

#ifndef FILE_H_
#define FILE_H_

#include <config.h>

#include <future/memory.hpp>
#include <future/string.hpp>
#include <stdexcept>

#include "libext/FileID.h"
#include "ResizableArray.h"


/**
 * A class to represent the contents and some metadata of a read-only file.
 * Abstracts away the method of access to the data, i.e. mmap() vs. read().
 */
class File
{
public:
	explicit File(std::shared_ptr<FileID> file_id, std::shared_ptr<ResizableArray<char>> storage = std::make_shared<ResizableArray<char>>());
	File(const std::string &filename, FileAccessMode fam, FileCreationFlag fcf,
			std::shared_ptr<ResizableArray<char>> storage = std::make_shared<ResizableArray<char>>());
	~File();

	[[nodiscard]] size_t size() const noexcept { return m_fileid->GetFileSize(); };

	[[nodiscard]] const char * data() const noexcept { return m_file_data; };

	/**
	 * Returns the name of this File as passed to the constructor.
	 * @return  The name of this File as passed to the constructor.
	 */
	[[nodiscard]] std::string name() const noexcept { return m_fileid->GetPath(); };

private:

	/**
	 * Return a pointer to a buffer containing the contents of the file described by #file_descriptor.
	 * May be mmap()'ed or read() into a newed buffer depending on m_use_mmap.
	 *
	 * @note file_descriptor will be closed after this function returns.
	 *
	 * @param file_descriptor  File descriptor (from open()) of the file to read in / mmap.
	 * @param file_size        Size of the file.
	 * @return
	 */
	const char* GetFileData(int file_descriptor, size_t file_size, size_t preferred_block_size);

	/**
	 * Frees the resources allocated by GetFileData().
	 *
	 * @param file_data  Pointer to the file data, as returned by GetFile().
	 * @param file_size  The same file_size used in the call to GetFile().
	 */
	void FreeFileData(const char * file_data, size_t file_size) noexcept;

	std::shared_ptr<FileID> m_fileid;

	/// The ResizableArray that we'll get file data storage from.
	std::shared_ptr<ResizableArray<char>> m_storage;

	const char *m_file_data { nullptr };

	bool m_use_mmap { false };

};

#endif /* FILE_H_ */
