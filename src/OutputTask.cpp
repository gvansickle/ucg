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

#include "OutputTask.h"

// Std C++.
#include <cstdio>
#include <iostream>

#include <unistd.h>

// Ours.
#include <libext/Logger.h>


OutputTask::OutputTask(bool flag_color, bool flag_nocolor, bool flag_column, bool flag_nullsep, sync_queue<MatchList> &input_queue)
  : m_input_queue(input_queue), m_enable_color(flag_color), m_print_column(flag_column), m_nullsep(flag_nullsep)
{
	m_output_context.reset(new OutputContext(m_enable_color, m_print_column, m_nullsep));
}

OutputTask::~OutputTask()
{
}

void OutputTask::Run()
{
	set_thread_name("OutputTask");

	MatchList ml;
	bool first_matchlist_printed = false;
	std::stringstream sstrm;

	while(m_input_queue.pull_front(std::move(ml)) != queue_op_status::closed)
	{
		if(first_matchlist_printed)
		{
			// Print a blank line between the match lists (i.e. the groups of matches in one file).
			std::cout << '\n';
		}
		ml.Print(sstrm, *m_output_context);
		std::cout << sstrm.str();
		std::cout.flush();
		sstrm.str(std::string());
		sstrm.clear();
		first_matchlist_printed = true;

		// Count up the total number of matches.
		m_total_matched_lines += ml.GetNumberOfMatchedLines();
	}
}

