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

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <utility>
#include <boost/thread/sync_queue.hpp>

#include "ArgParse.h"
#include "Globber.h"
#include "TypeManager.h"
#include "MatchList.h"
#include "FileScanner.h"
#include "OutputTask.h"



int main(int argc, char **argv)
{
	std::vector<std::thread> scanner_threads;

	TypeManager tm;

	ArgParse ap;

	// Parse command-line options and args.
	ap.Parse(argc, argv);

	tm.CompileTypeTables();

	std::clog << "Num jobs: " << ap.m_jobs << std::endl;

	// Create the queues.
	boost::concurrent::sync_queue<std::string> q;
	boost::concurrent::sync_queue<MatchList> out_q;

	// Set up the globber.
	Globber g(ap.m_paths, tm, q);
	// Set up the output task.
	OutputTask output_task(ap.m_color, out_q);

	// Start the globber thread.
	std::thread gt {&Globber::Run, &g};

	// Start the output task thread.
	std::thread ot {&OutputTask::Run, &output_task};



	// Start the scanner threads.
	FileScanner fs(q, out_q, ap.m_pattern, ap.m_ignore_case);
	for(int t=0; t<ap.m_jobs; ++t)
	{
		std::thread fst {&FileScanner::Run, &fs};
		scanner_threads.push_back(std::move(fst));
	}

	// Wait for the Globber thread (the source) to finish.
	gt.join();
	// Close the Globber->FileScanner queue.
	q.close();

	// Wait for all scanner threads to complete.
	for (auto& th : scanner_threads)
	{
		th.join();
	}
	// All scanner threads completed.

	// Close the FileScanner->OutputTask queue.
	out_q.close();

	// Wait for the output thread to complete.
	ot.join();

	return 0;
}
