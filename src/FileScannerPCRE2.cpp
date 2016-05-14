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

#include "FileScannerPCRE2.h"

FileScannerPCRE2::FileScannerPCRE2(sync_queue<std::string> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : FileScanner(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal)
{
#ifdef HAVE_LIBPCRE2
	// Compile the regex.
	int error;
	PCRE2_SIZE error_offset;
	uint32_t options = 0;

	// For now, we won't support capturing.  () will be treated as (?:).
	options = PCRE2_NO_AUTO_CAPTURE;

	if(ignore_case)
	{
		// Ignore case while matching.
		options |= PCRE2_CASELESS;
	}

	if(m_pattern_is_literal)
	{
		// Surround the pattern with \Q...\E so it's treated as a literal string.
		regex = "\\Q" + regex + "\\E";
	}

	if(m_word_regexp)
	{
		// Surround the regex with \b (word boundary) assertions.
		regex = "\\b(?:" + regex + ")\\b";
	}

	m_pcre2_regex = pcre2_compile(static_cast<PCRE2_SPTR8>(regex.c_str()), regex.length(), options, &error, &error_offset, NULL);

#endif
}

FileScannerPCRE2::~FileScannerPCRE2()
{
	pcre2_code_free(m_pcre2_regex);
}

void FileScannerPCRE2::ScanFile(const char* file_data, size_t file_size,
		MatchList& ml)
{

}
