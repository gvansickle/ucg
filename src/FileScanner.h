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

#ifndef FILESCANNER_H_
#define FILESCANNER_H_


#include <string>

#include "sync_queue_impl_selector.h"
#include "MatchList.h"

/**
 * Class which does the actual regex scanning of a file.
 */
class FileScanner
{
public:
	FileScanner(sync_queue<std::string> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp);
	virtual ~FileScanner();

	void Run();

private:

	/**
	 * Helper to assign each thread which starts Run() to a different core.
	 * Note that this currently only works on Linux, and does not appear to make
	 * a measurable difference in performance, likely because we're I/O bound regardless.
	 *
	 * Maintaining this for experimental purposes.
	 */
	void AssignToNextCore();

	sync_queue<std::string>& m_in_queue;

	sync_queue<MatchList> &m_output_queue;

	std::string m_regex;

	bool m_ignore_case;

	bool m_word_regexp;

	int m_next_core;

	bool m_use_mmap;

	/**
	 * Switch to make Run() assign its std::thread to different cores on the machine.
	 * If false, the underlying std::thread logic is allowed to decide which threads run on
	 * which cores.  See note above for AssignToNextCore(); turning this on makes no real difference.
	 *
	 * Maintaining this for experimental purposes.
	 */
	bool m_manually_assign_cores;
};

#endif /* FILESCANNER_H_ */
