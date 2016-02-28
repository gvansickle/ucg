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

#include "OutputTask.h"

#include <unistd.h>
#include <stdio.h>
#include <iostream>

OutputTask::OutputTask(bool flag_color, bool flag_nocolor, bool flag_column, sync_queue<MatchList> &input_queue)
	: m_input_queue(input_queue)
{
	// Determine if the output is going to a terminal.  If so we'll use color by default, group the matches under
	// the filename, etc.
	m_output_is_tty = isatty(fileno(stdout));

	// Determine whether to enable color or not.
	// Color is enabled if explicitly specified with --color or
	// if outputting to a TTY and --nocolor is not specified.
	if(flag_color || (!flag_nocolor && m_output_is_tty))
	{
		m_enable_color = true;
	}
	else
	{
		m_enable_color = false;
	}

	m_print_column = flag_column;
}

OutputTask::~OutputTask()
{
}

void OutputTask::Run()
{
	MatchList ml;
	bool first_matchlist_printed = false;

	while(m_input_queue.wait_pull(ml) != queue_op_status::closed)
	{
		if(first_matchlist_printed && m_output_is_tty)
		{
			// Print a blank line between the match lists (i.e. the groups of matches in one file).
			std::cout << std::endl;
		}
		ml.Print(m_output_is_tty, m_enable_color, m_print_column);
		first_matchlist_printed = true;

		// Count up the total number of matches.
		m_total_matched_lines += ml.GetNumberOfMatchedLines();
	}
}
