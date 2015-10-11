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

#include "OutputTask.h"

#include <unistd.h>
#include <stdio.h>

OutputTask::OutputTask(bool enable_color, sync_queue<MatchList> &input_queue)
	: m_input_queue(input_queue), m_enable_color(enable_color)
{
	m_output_is_tty = isatty(fileno(stdout));
}

OutputTask::~OutputTask()
{
	// TODO Auto-generated destructor stub
}

void OutputTask::Run()
{
	MatchList ml;

	while(m_input_queue.wait_pull(ml) != queue_op_status::closed)
	{
		ml.Print(m_output_is_tty, m_enable_color);
	}
}
