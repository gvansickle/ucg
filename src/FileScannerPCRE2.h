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

#ifndef SRC_FILESCANNERPCRE2_H_
#define SRC_FILESCANNERPCRE2_H_

#include <config.h>

#include "FileScanner.h"

#ifdef HAVE_LIBPCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

class FileScannerPCRE2: public FileScanner
{
public:
	FileScannerPCRE2(sync_queue<FileID> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp,
			bool pattern_is_literal);
	virtual ~FileScannerPCRE2();

private:

	/**
	 * Scan @a file_data for matches of m_pcre2_regex using libpcre2.  Add hits to @a ml.
	 *
	 * @param file_data
	 * @param file_size
	 * @param ml
	 */
	void ScanFile(const char * __restrict__ file_data, size_t file_size, MatchList &ml) override final;

	std::string PCRE2ErrorCodeToErrorString(int errorcode);

#ifdef HAVE_LIBPCRE2
	/// The compiled libpcre2 regex.
	/// @todo Make this a unique_ptr<>, RAII-ify it.
	//std::unique_ptr<pcre2_code, void(*)(pcre2_code*)> m_pcre2_regex;
	pcre2_code *m_pcre2_regex;
#endif
};

#endif /* SRC_FILESCANNERPCRE2_H_ */
