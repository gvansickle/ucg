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

#include "MatchList.h"

#include <iostream>

MatchList::MatchList(const std::string &filename) : m_filename(filename)
{

}

MatchList::~MatchList()
{
}

void MatchList::AddMatch(const Match &match)
{
	m_match_list.push_back(match);
}

void MatchList::Print(bool istty, bool enable_color, bool print_column) const
{
	std::string no_dotslash_fn;

	// If the file path starts with a "./", chop it off.
	// This is to match the behavior of ack.
	if(m_filename.find("./") == 0)
	{
		no_dotslash_fn = std::string(m_filename.begin()+2, m_filename.end());
	}
	else
	{
		no_dotslash_fn = std::string(m_filename.begin(), m_filename.end());
	}

	std::string color_filename("\x1B[32;1m"); // 32=green, 1=bold
	std::string color_match("\x1B[30;43;1m"); // 30=black, 43=yellow bkgnd, 1=bold
	std::string color_lineno("\x1B[33;1m");   // 33=yellow, 1=bold
	std::string color_default("\x1B[0m");

	if(!enable_color)
	{
		color_filename = "";
		color_match = "";
		color_lineno = "";
		color_default = "";
	}

	if(istty)
	{
		// Render to a TTY device.

		std::cout << color_filename << no_dotslash_fn << color_default << std::endl;
		for(auto it : m_match_list)
		{
			std::cout << color_lineno << it.m_line_number << color_default << ":";
			if(print_column)
			{
				std::cout << it.m_pre_match.length()+1 << ":";
			}
			std::cout << it.m_pre_match << color_match << it.m_match << color_default << it.m_post_match << std::endl;
		}
	}
	else
	{
		// Render to a pipe or file.

		for(auto it : m_match_list)
		{
			std::cout << color_filename << no_dotslash_fn << color_default << ":"
					<< color_lineno << it.m_line_number << color_default << ":";
			if(print_column)
			{
				std::cout << it.m_pre_match.length()+1 << ":";
			}
			std::cout << it.m_pre_match << color_match << it.m_match << color_default << it.m_post_match << std::endl;
		}
	}
}

std::vector<Match>::size_type MatchList::GetNumberOfMatchedLines() const
{
	// One Match in the MatchList equals one matched line.
	return m_match_list.size();
}
