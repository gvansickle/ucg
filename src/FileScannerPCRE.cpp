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

#include "FileScannerPCRE.h"

#include <iostream>
#include <libext/string.hpp>

#include "Logger.h"

#ifdef HAVE_LIBPCRE

/// Old enough libpcre's don't have this define.
#ifndef PCRE_NEVER_UTF
#define PCRE_NEVER_UTF 0
#endif


/**
 * This callout handler is invoked by PCRE at the end of a potentially successful match.  It's purpose
 * is to prevent a regex like 'abc\s+def' from matching across an eol boundary, since '\s' matches both
 * 'normal' spaces and also newlines.
 *
 * It works in conjunction with a wrapper the constructor puts around the incoming regex, "(?:" + regex + ")(?=.*?$)(?C1)".
 * What happens is that when PCRE finds a potential match of the given regex, the (?C1) causes this function to be called.
 * This function then scans the potential match for a '\n' character.  If it finds one, the potential match is rejected by returning
 * a positive integer (+1), and if it's able to, PCRE backtracks and looks for a different match solution.  If no
 * '\n' is found, 0 is returned, and the match is accepted.
 *
 * @param cob
 * @return
 */
static int callout_handler(pcre_callout_block *cob)
{
	const char * p = (const char *)std::memchr(cob->subject+cob->start_match, '\n', cob->current_position - cob->start_match);

	std::string cur_match_str((const char *)cob->subject+cob->start_match, (const char *)cob->subject + cob->current_position);

	if(p == nullptr)
	{
		//std::cerr << "CALLOUT: No eols yet, string is: \"" << cur_match_str << "\"\n";
		return 0;
	}
	else
	{
		//std::cerr << "CALLOUT: Found eol, string is: \"" << cur_match_str << "\"\n";
		return 1;
	}
}
#endif

FileScannerPCRE::FileScannerPCRE(sync_queue<FileID> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : FileScanner(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal)
{
#ifdef HAVE_LIBPCRE
	// Compile the regex.
	const char *error;
	int error_offset;
	int regex_compile_options = 0;

	// For now, we won't support capturing.  () will be treated as (?:).
	regex_compile_options = PCRE_NO_AUTO_CAPTURE | PCRE_MULTILINE | PCRE_NEVER_UTF;

	if(ignore_case)
	{
		// Ignore case while matching.
		regex_compile_options |= PCRE_CASELESS;
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

	// Put in our callout, which essentially exists to make '\s' not match a newline.
	regex = "(?:" + regex + ")(?=.*?$)(?C1)";
		
	// PCRE1 has a single global pointer to the callout function.  This is OK for our use here.
	pcre_callout = callout_handler;

	m_pcre_regex = pcre_compile(regex.c_str(), regex_compile_options, &error, &error_offset, NULL);

	if (m_pcre_regex == NULL)
	{
		// Regex compile failed, we can't continue.
		throw FileScannerException(std::string("Compilation of regex \"") + regex + "\" failed at offset " + std::to_string(error_offset) + ": " + error);
	}

	m_pcre_extra = pcre_study(m_pcre_regex, PCRE_STUDY_JIT_COMPILE, &error);

	if(error != NULL)
	{
		// Study error.
		pcre_free(m_pcre_regex);
		throw FileScannerException(std::string("PCRE study error: ") + *error);
	}
#endif
}

FileScannerPCRE::~FileScannerPCRE()
{
#ifdef HAVE_LIBPCRE
	pcre_free_study(m_pcre_extra);
	pcre_free(m_pcre_regex);
#endif
}

void FileScannerPCRE::ScanFile(const char* __restrict__ file_data, size_t file_size, MatchList& ml)
{
#ifdef HAVE_LIBPCRE
	// Match output vector.  We won't support submatches, so we only need two entries, plus a third for pcre's own use.
	int ovector[3] = {-1, 0, 0};
	size_t line_no = 1;
	size_t prev_lineno = 0;
	const char *prev_lineno_search_end = file_data;
	// Up-cast file_size, which is a size_t (unsigned) to a ptrdiff_t (signed) which should be able to handle the
	// same positive range, and not cause issues when compared with the ints of ovector[].
	std::ptrdiff_t signed_file_size = file_size;

	// Loop while the start_offset is less than the file_size.
	while(ovector[1] < signed_file_size)
	{
		int options = 0;
		int start_offset = ovector[1];

		// Was the previous match zero-length?
		if (ovector[0] == ovector[1])
		{
			// Yes, are we at the end of the file?
			if (ovector[0] == signed_file_size)
			{
				// Yes, we're done searching.
				break;
			}

			// Not done, set options for another try for a non-empty match at the same point.
			options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
		}

		// Try to match the regex to whatever's left of the file.
		int rc = pcre_exec(
				m_pcre_regex,
				m_pcre_extra,
				file_data,
				file_size,
				start_offset,
				options,
				ovector,
				3);

		// Check for no match.
		if(rc == PCRE_ERROR_NOMATCH)
		{
			if(options == 0)
			{
				// We weren't trying to recover from a zero-length match, so there are no more matches.
				// Break out of the loop.
				break;
			}
			else
			{
				// We've failed to find a non-empty-string match at a point where
				// we previously found an empty-string match.
				// Advance one character and continue.
				ovector[1] = start_offset + 1;

				/**
				 * @todo If we're treating \r\n as a newline, we have to check here to see
				 *       if we are at the start of one, and if so, skip over the whole thing.
				 *       For now, we don't support this.
				 */
				if(/** @todo crlf_is_newline */ false &&
						start_offset < file_size-1 &&
						file_data[start_offset] == '\r' &&
						file_data[start_offset+1] == '\n')
				{
					// Increment the new start position by one more byte, we're at a \r\n line ending.
					ovector[1]++;
				}
				/**
				 * @todo Similarly, if we support UTF-8, we have to skip all bytes in the
				 *       possibly multi-byte character.
				 *       Again, UTF-8 is not something we support at the moment.
				 */
				else if(false /** @todo utf8 */)
				{
					// Increment a whole UTF8 character.
					while(ovector[1] < signed_file_size)
					{
						if((file_data[ovector[1]] & 0xC0) != 0x80)
						{
							// Found a non-start-byte.
							break;
						}
						else
						{
							// Go to the next byte in the character.
							ovector[1]++;
						}
					}
				}
			}

			// Try to match again.
			continue;
		}

		// Check for non-PCRE_ERROR_NOMATCH error codes.
		if(rc < 0)
		{
			// Match error.  Convert to string, throw exception.
			throw FileScannerException(std::string("PCRE match error: ") + std::to_string(rc));
			return;
		}
		if (rc == 0)
		{
			ERROR() << "ovector only has room for 1 captured substring" << std::endl;
			return;
		}

		// There was a match.  Package it up in the MatchList which was passed in.
		line_no += CountLinesSinceLastMatch(prev_lineno_search_end, file_data+ovector[0]);
		prev_lineno_search_end = file_data+ovector[0];
		if(line_no == prev_lineno)
		{
			// Skip multiple matches on one line.
			continue;
		}
		prev_lineno = line_no;
		Match m(file_data, file_size, ovector[0], ovector[1], line_no);

		ml.AddMatch(std::move(m));
	}
#endif // HAVE_LIBPCRE
}
