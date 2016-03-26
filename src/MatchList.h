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

#ifndef MATCHLIST_H_
#define MATCHLIST_H_

#include <string>
#include <vector>

#include "Match.h"

/*
 *
 */
class MatchList
{
public:
	MatchList(const std::string &filename);
	MatchList() {};

	MatchList(const MatchList &lvalue) = delete;
	MatchList(MatchList&&) noexcept = default;
	MatchList& operator=(MatchList&&) = default;
#if 0
	~MatchList();
#endif

	void AddMatch(Match &&match);

	void Print(bool istty, bool enable_color, bool print_column) const;

	/// @todo GRVS - This needs to return 'empty' after a move-from has occurred.
	bool empty() const noexcept { return m_match_list.empty(); };

	std::vector<Match>::size_type GetNumberOfMatchedLines() const;

private:

	/// The filename where the Matches in this MatchList were found.
	std::string m_filename;

	/// The Matches.
	std::vector<Match> m_match_list;
};

// Require MatchList to be nothrow move constructible so that a container of them can use move on reallocation.
static_assert(std::is_nothrow_move_constructible<MatchList>::value == true, "MatchList must be nothrow move constructible");

// Require Match to not be copy constructible, so that uses don't end up accidentally copying it instead of moving.
static_assert(std::is_copy_constructible<MatchList>::value == false, "MatchList must not be copy constructable");

#endif /* MATCHLIST_H_ */
