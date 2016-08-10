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

#include "MatchList.h"

#include <iostream>
#include <sstream>

MatchList::MatchList(const std::string &filename) : m_filename(filename)
{

}

void MatchList::AddMatch(Match &&match)
{
	m_match_list.push_back(std::move(match));
}

void MatchList::Print(std::ostream &sstrm, OutputContext &output_context) const
{
	std::string no_dotslash_fn;
	const std::string empty_color_string {""};

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

	const std::string *color_filename { &empty_color_string };
	const std::string *color_match { &empty_color_string };
	const std::string *color_lineno { &empty_color_string };
	const std::string *color_default { &empty_color_string };

	if(output_context.is_color_enabled())
	{
		color_filename = &output_context.m_color_filename;
		color_match = &output_context.m_color_match;
		color_lineno = &output_context.m_color_lineno;
		color_default = &output_context.m_color_default;
	}

	//std::string composition_buffer;

	// The only real difference between TTY vs. non-TTY printing here is that for TTY we print:
	//   filename
	//   lineno:column:match
	//   [...]
	// while for non-TTY we print:
	//   filename:lineno:column:match
	//   [...]
	if(output_context.is_output_tty())
	{
		// Render to a TTY device.

		sstrm << *color_filename + no_dotslash_fn + *color_default + "\n";
		for(const Match& it : m_match_list)
		{
			sstrm << *color_lineno << it.m_line_number << *color_default + ":";
			if(output_context.is_column_print_enabled())
			{
				sstrm << it.m_pre_match.length()+1 << ":";
			}
			sstrm << it.m_pre_match + *color_match + it.m_match + *color_default + it.m_post_match + '\n';
		}
	}
	else
	{
		// Render to a pipe or file.

		for(const Match& it : m_match_list)
		{
			sstrm << *color_filename + no_dotslash_fn + *color_default + ":"
					+ *color_lineno << it.m_line_number << *color_default + ":";
			if(output_context.is_column_print_enabled())
			{
				sstrm << it.m_pre_match.length()+1 << ":";
			}
			sstrm << it.m_pre_match + *color_match + it.m_match << *color_default + it.m_post_match + '\n';
		}
	}
}

std::vector<Match>::size_type MatchList::GetNumberOfMatchedLines() const noexcept
{
	// One Match in the MatchList equals one matched line.
	return m_match_list.size();
}
