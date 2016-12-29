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

#include "FileScannerCpp11.h"

FileScannerCpp11::FileScannerCpp11(sync_queue<std::shared_ptr<FileID>> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : FileScanner(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal)
{
#ifdef USE_CXX11_REGEX
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
}

FileScannerCpp11::~FileScannerCpp11()
{
}

void FileScannerCpp11::ScanFile( const char * __restrict__ file_data [[maybe_unused]], size_t file_size [[gnu::unused]], MatchList &ml [[gnu::unused]])
{
#ifdef USE_CXX11_REGEX
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
