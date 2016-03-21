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

#ifndef MATCH_H_
#define MATCH_H_

#include <string>

/*
 *
 */
class Match
{
public:
	Match(const char *start_of_array, size_t array_size, size_t match_start_offset, size_t match_end_offset, long long line_number);
	Match() = default;
#if 0
	Match(const Match &other) = default;
	~Match() { };
#endif

	/// @note Data members not private, this is more of a struct than a class.
	long long m_line_number { 0 };
	std::string m_pre_match;
	std::string m_match;
	std::string m_post_match;
};

#endif /* MATCH_H_ */
