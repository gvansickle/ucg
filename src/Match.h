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

/** @file Match.h */

#ifndef MATCH_H_
#define MATCH_H_

#include <string>
#include <type_traits>

/**
 * Class representing a single match in a single file found by FileScanner::ScanFile*().
 * Mostly struct-like behavior; e.g. all data members are public, no member functions other than the constructors.
 */
class Match
{
public:
	Match(const char *start_of_array, size_t array_size, size_t match_start_offset, size_t match_end_offset, long long line_number);
	Match() = default;

	/// Delete the copy constructor and the move assignment operator.  With the std::strings in here, this is a relatively expensive
	/// class to copy, so we only allow move-constructing.
	Match(const Match &other) = delete;
	Match& operator=(const Match&) = delete;

	/// Since we deleted the copy constructor, we have to explicitly declare that we want the default move ctor and assignment op.
	Match(Match &&other) noexcept = default;
	Match& operator=(Match &&) noexcept = default;

	/// Also use the default destructor.
	~Match() = default;


	/// @note Data members not private, this is more of a struct than a class.
	long long m_line_number { 0 };
	std::string m_pre_match;
	std::string m_match;
	std::string m_post_match;
};

// Require Match to be nothrow move constructible so that a vector of them can use move on reallocation.
static_assert(std::is_nothrow_move_constructible<Match>::value == true, "Match must be nothrow move constructible");

// Require Match to not be copy constructible, so that uses don't end up accidentally copying it instead of moving.
static_assert(std::is_copy_constructible<Match>::value == false, "Match must not be copy constructible");

#endif /* MATCH_H_ */
