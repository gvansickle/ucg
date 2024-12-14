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

#ifndef OUTPUTTASK_H_
#define OUTPUTTASK_H_

#include <config.h>

#include <MatchList.h>

#include <memory>

#include "sync_queue_impl_selector.h"
#include "OutputContext.h"

/**
 * Task which serializes the output from the FileScanner threads.
 */
class OutputTask
{
public:
      OutputTask(bool flag_color, bool flag_nocolor, bool flag_column, bool flag_nullsep, sync_queue<MatchList> &input_queue);
	virtual ~OutputTask();

	void Run();

	[[nodiscard]] long long GetTotalMatchedLines() const { return m_total_matched_lines; };

private:

	/// The queue from which we'll pull our MatchLists.
	sync_queue<MatchList> &m_input_queue;

	/// Whether to output color or not.  Determined in constructor.
	bool m_enable_color;

	/// Whether to print the column number of the first match or not.
	bool m_print_column;

	/// Whether to write a null after a filename instead of ':'.
	bool m_nullsep;

	std::unique_ptr<OutputContext> m_output_context;

	/// The total number of matched lines as reported by the incoming MatchLists.
	long long m_total_matched_lines { 0 };
};

#endif /* OUTPUTTASK_H_ */
