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

#ifndef OUTPUTTASK_H_
#define OUTPUTTASK_H_

#include <MatchList.h>
#include <boost/thread/sync_queue.hpp>

/*
 *
 */
class OutputTask
{
public:
	OutputTask(bool enable_color, boost::concurrent::sync_queue<MatchList> &input_queue);
	virtual ~OutputTask();

	void Run();

private:

	boost::concurrent::sync_queue<MatchList> &m_input_queue;

	bool m_output_is_tty;

	bool m_enable_color;
};

#endif /* OUTPUTTASK_H_ */
