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
#include "future/string.hpp"


void MatchList::SetFilename(std::string filename)
{
	m_filename = std::move(filename);
}

void MatchList::AddMatch(Match &&match)
{
	m_match_list.push_back(std::move(match));
}

void MatchList::clear() noexcept
{
	m_filename.clear();
	m_match_list.clear();
}

void MatchList::Print(std::ostream &sstrm, OutputContext &output_context) const
{
	std::string_view no_dotslash_fn;
	static constexpr std::string empty_color_string {""};
	bool color = output_context.is_color_enabled();

	// If the file path starts with a "./", chop it off.
	// This is to match the behavior of ack.
	if(m_filename.find("./") == 0)
	{
		no_dotslash_fn = std::string_view(m_filename.begin()+2, m_filename.end());
	}
	else
	{
		no_dotslash_fn = std::string_view(m_filename.begin(), m_filename.end());
	}

	const std::string *color_filename { &empty_color_string };
	const std::string *color_match { &empty_color_string };
	const std::string *color_lineno { &empty_color_string };
	const std::string *color_default { &empty_color_string };

	if(color)
	{
		color_filename = &output_context.m_color_filename;
		color_match = &output_context.m_color_match;
		color_lineno = &output_context.m_color_lineno;
		color_default = &output_context.m_color_default;
	}

	std::string composition_buffer;
	composition_buffer.reserve(256);

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

		// Print file header.
		if(color) composition_buffer += *color_filename;
		composition_buffer += no_dotslash_fn;
		if(color) composition_buffer += *color_default;
		composition_buffer += '\n';
		sstrm << composition_buffer;

		// Print the individual matches.
		for(const Match& it : m_match_list)
		{
			composition_buffer.clear();
			if(color) composition_buffer += *color_lineno;
			composition_buffer += std::to_string(it.m_line_number);
			if(color) composition_buffer += *color_default;
			composition_buffer += ':';
			sstrm << composition_buffer;
			if(output_context.is_column_print_enabled())
			{
				sstrm << it.m_pre_match.length()+1 << ':';
			}
			composition_buffer.clear();
			composition_buffer += it.m_pre_match;
			if(color) composition_buffer += *color_match;
			composition_buffer += it.m_match;
			if(color) composition_buffer += *color_default;
			composition_buffer += it.m_post_match;
			composition_buffer += '\n';
			sstrm << composition_buffer;
		}
	}
	else
	{
		// Render to a pipe or file.

		for(const Match& it : m_match_list)
		{
			// Print file name at the beginning of each line.
			composition_buffer.clear();
			if(color) composition_buffer += *color_filename;
			composition_buffer += no_dotslash_fn;
			if(color) composition_buffer += *color_default;
			composition_buffer += ':';

			// Line number.
			if(color) composition_buffer += *color_lineno;
			composition_buffer += std::to_string(it.m_line_number);
			if(color) composition_buffer += *color_default;
			composition_buffer += ':';

			sstrm << composition_buffer;

			// The column, if enabled.
			if(output_context.is_column_print_enabled())
			{
				sstrm << it.m_pre_match.length()+1 << ':';
			}

			// The match text.
			composition_buffer.clear();
			composition_buffer += it.m_pre_match;
			if(color) composition_buffer += *color_match;
			composition_buffer += it.m_match;
			if(color) composition_buffer += *color_default;
			composition_buffer += it.m_post_match;
			composition_buffer += '\n';
			sstrm << composition_buffer;
		}
	}
}

std::vector<Match>::size_type MatchList::GetNumberOfMatchedLines() const noexcept
{
	// One Match in the MatchList equals one matched line.
	return m_match_list.size();
}
