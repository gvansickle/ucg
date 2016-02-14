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

#include "FileScanner.h"
#include "File.h"
#include "Match.h"
#include "MatchList.h"

#include "config.h"

#include <iostream>
#include <string>
#ifdef HAVE_LIBPCRE
#include <pcre.h>
#endif
#include <regex>
#include <sstream>
#include <thread>
#include <mutex>
#ifndef HAVE_SCHED_SETAFFINITY
#else
	#include <sched.h>
#endif

static std::mutex f_assign_affinity_mutex;

FileScanner::FileScanner(sync_queue<std::string> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex),
				m_ignore_case(ignore_case), m_word_regexp(word_regexp), m_pattern_is_literal(pattern_is_literal),
				m_next_core(0), m_use_mmap(false), m_manually_assign_cores(false)
{
#ifdef HAVE_LIBPCRE
	// Compile the regex.
	const char *error;
	int error_offset;
	int options = 0;

	// For now, we won't support capturing.  () will be treated as (?:).
	options = PCRE_NO_AUTO_CAPTURE;

	if(ignore_case)
	{
		// Ignore case while matching.
		options |= PCRE_CASELESS;
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

	m_pcre_regex = pcre_compile(regex.c_str(), options, &error, &error_offset, NULL);

	if (m_pcre_regex == NULL)
	{
		// Regex compile failed, we can't continue.
		/// @todo GRVS - This should be as simple as putting a C++11 "std::to_string(error_offset)" into the string below.
		///              However, there's an issue with at least Cygwin's std lib and/or gcc itself which makes to_string() unavailable
		/// 			 (see e.g. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61580 (fixed on gcc trunk 2015-11-13),
		/// 			 https://sourceware.org/ml/cygwin/2015-01/msg00251.html).  Since I don't want to wait for the fix to trickle
		/// 			 out and I don't know how widespread the issue is, we'll do it the old-fashioned way.
		std::ostringstream ss;
		ss << error_offset;
		throw FileScannerException(std::string("Compilation of regex \"") + regex + "\" failed at offset " + ss.str() + ": " + error);
	}

	m_pcre_extra = pcre_study(m_pcre_regex, PCRE_STUDY_JIT_COMPILE, &error);

	if(error != NULL)
	{
		// Study error.
		pcre_free(m_pcre_regex);
		throw FileScannerException("PCRE study error: " + *error);
	}
#endif
}

FileScanner::~FileScanner()
{
#ifdef HAVE_LIBPCRE
	pcre_free_study(m_pcre_extra);
	pcre_free(m_pcre_regex);
#endif
}

void FileScanner::Run()
{
	if(m_manually_assign_cores)
	{
		// Spread the scanner threads across cores.  Linux at least doesn't seem to want to do that by default.
		AssignToNextCore();
	}

#ifndef HAVE_LIBPCRE
	// Create the std::regex we're looking for, possibly ignoring case, possibly with match-whole-word.
	auto stack_regex = m_regex;
	if(m_word_regexp)
	{
		// Surround the regex with \b (word boundary) assertions.
		stack_regex = "\\b(?:" + m_regex + ")\\b";
	}
	std::regex expression(stack_regex,
			std::regex_constants::ECMAScript |
			std::regex_constants::optimize   |
			m_ignore_case ? std::regex_constants::icase : static_cast<typeof(std::regex_constants::icase)>(0));
#endif

	// Pull new filenames off the input queue until it's closed.
	std::string next_string;
	while(m_in_queue.wait_pull(next_string) != queue_op_status::closed)
	{
		MatchList ml(next_string);

		try
		{
			// Try to open and read the file.  This could throw.
			File f(next_string);

			if(f.size() == 0)
			{
				//std::clog << "WARNING: Filesize of \"" << next_string << "\" is 0" << std::endl;
				continue;
			}

			const char *file_data = f.data();
			size_t file_size = f.size();

			// Scan the file data for the regex.
#if HAVE_LIBPCRE
			ScanFileLibPCRE(file_data, file_size, ml);
#else
			ScanFileCpp11(expression, file_data, file_size, ml);
#endif

			if(!ml.empty())
			{
				/// @todo Move semantics here?
				m_output_queue.wait_push(ml);
			}
		}
		catch(const FileException &error)
		{
			// The File constructor threw an exception.
			std::cerr << "ucg: ERROR: " << error.what() << std::endl;
		}
		catch(const std::system_error& error)
		{
			// A system error.  Currently should only be errors from File.
			std::cerr << "ucg: ERROR: " << error.code() << " - " << error.code().message() << std::endl;
		}
		catch(...)
		{
			// Rethrow whatever it was.
			throw;
		}
	}
}

void FileScanner::AssignToNextCore()
{
#ifdef HAVE_SCHED_SETAFFINITY

	// Prevent the multiple threads from stepping on each other and screwing up m_next_core.
	std::lock_guard<std::mutex> lg {f_assign_affinity_mutex};

	cpu_set_t cpuset;

	// Clear the cpu_set_t.
	CPU_ZERO(&cpuset);

	// Set the bit of the next CPU.
	CPU_SET(m_next_core, &cpuset);

	sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	// Increment so we use the next core the next time.
	m_next_core++;
	m_next_core %= std::thread::hardware_concurrency();
#endif
}

void FileScanner::ScanFileCpp11(const std::regex& /*expression*/, const char */*file_data*/, size_t /*file_size*/, MatchList& /*ml*/)
{
#ifndef HAVE_LIBPCRE
	// Scan the mmapped file for the regex.
	std::regex_iterator<const char *> rit(file_data, file_data+file_size, expression);
	std::regex_iterator<const char *> rend;
	while(rit != rend)
	{
		//std::cout << "Match in file " << next_string << std::endl;

		long long lineno = 1+std::count(file_data, file_data+rit->position(), '\n');
		auto line_ending = "\n";
		auto line_start = std::find_end(file_data, file_data+rit->position(),
				line_ending, line_ending+1);
		if(line_start == file_data+rit->position())
		{
			// The line has no starting '\n', so it must be the first line.
			line_start = file_data;
		}
		else
		{
			// The line had a starting '\n', clip it off.
			++line_start;
		}
		auto line_end = std::find(file_data+rit->position(), file_data+file_size, '\n');
		auto pre_match = std::string(line_start, file_data+rit->position());
		auto match = std::string(rit->begin()->str());
		auto post_match = std::string(file_data+rit->position()+rit->length(), line_end);
		Match m = { pre_match, match, post_match };
		ml.AddMatch(lineno, m);

		++rit;
	}
#endif
}

void FileScanner::ScanFileLibPCRE(const char *file_data, size_t file_size, MatchList& ml)
{
	// Match output vector.  We won't support submatches, so we only need two entries, plus a third for pcre's own use.
	int ovector[3] = {-1, 0, 0};
	long long line_no = 1;
	long long prev_lineno = 0;
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
						start_offset < file_size -1 &&
						file_data[start_offset] == '\r' &&
						file_data[start_offset] == '\n')
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
			std::cerr << "ERROR: Match error " << rc << "." << std::endl;
			return;
		}
		if (rc == 0)
		{
			std::cerr << "ERROR: ovector only has room for 1 captured substring" << std::endl;
			return;
		}

		// There was a match.  Package it up in the MatchList which was passed in.
		long long num_lines_since_last_match = std::count(prev_lineno_search_end, file_data+ovector[0], '\n');
		line_no += num_lines_since_last_match;
		prev_lineno_search_end = file_data+ovector[0];
		if(line_no == prev_lineno)
		{
			// Skip multiple matches on one line.
			continue;
		}
		prev_lineno = line_no;
		Match m(file_data, file_size, ovector[0], ovector[1], line_no);

		ml.AddMatch(m);
	}
}
