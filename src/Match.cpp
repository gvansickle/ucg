/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "Match.h"

#include <algorithm>

Match::Match(const char *start_of_array, size_t array_size, size_t match_start_offset, size_t match_end_offset, long long line_number)
{
	auto line_ending = "\n";
	// Find the start of the line.
	std::reverse_iterator<const char*> rstart(start_of_array+match_start_offset);
	std::reverse_iterator<const char*> rend(start_of_array);

	auto line_start_rit = std::find(rstart, rend, line_ending[0]);
	const char *line_start;
	if(line_start_rit == rend)
	{
		// The line has no starting '\n', so it must be the first line.
		line_start = start_of_array;
	}
	else
	{
		// The line had a starting '\n', clip it off.
		line_start = line_start_rit.base();
	}

	// Find the end of the matched line.
	auto line_end = std::find(start_of_array+match_start_offset, start_of_array+array_size, line_ending[0]);

	// Form the match substrings.
	m_pre_match = std::string(line_start, start_of_array+match_start_offset);
	m_match = std::string(start_of_array+match_start_offset, start_of_array+match_end_offset);
	m_post_match = std::string(start_of_array+match_start_offset+(match_end_offset-match_start_offset), line_end);
	m_line_number = line_number;
}

