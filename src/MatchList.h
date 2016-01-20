/*
 * Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <map>

#if 0
struct Match
{
	std::string m_pre_match;
	std::string m_match;
	std::string m_post_match;
};
#endif

/*
 *
 */
class MatchList
{
public:
	MatchList() {};
	MatchList(const MatchList &lvalue) = default;
	MatchList(const std::string &filename);
	virtual ~MatchList();

	void AddMatch(long long lineno, const Match &match);

	void Print(bool istty, bool enable_color);

	bool empty() const noexcept { return m_match_list.empty(); };

private:

	std::string m_filename;

	std::map<long long, Match> m_match_list;
};

#endif /* MATCHLIST_H_ */
