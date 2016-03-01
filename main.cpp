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

#include "sync_queue_impl_selector.h"

#include "ArgParse.h"
#include "Globber.h"
#include "TypeManager.h"
#include "DirInclusionManager.h"
#include "MatchList.h"
#include "FileScanner.h"
#include "OutputTask.h"


int main(int argc, char **argv)
{
	try
	{
		// We'll keep the scanner threads in this vector so we can join() them later.
		std::vector<std::thread> scanner_threads;

		// Instantiate classes for file and directory inclusion/exclusion management.
		TypeManager tm;
		DirInclusionManager dim;

		// Instantiate the argument parser.
		ArgParse ap(tm);

		// Parse command-line options and args.
		ap.Parse(argc, argv);

		dim.AddExclusions(ap.m_excludes);

		tm.CompileTypeTables();
		dim.CompileExclusionTables();

		//std::clog << "Num jobs: " << ap.m_jobs << std::endl;

		// Create the Globber->FileScanner queue.
		sync_queue<std::string> q;

		// Create the FileScanner->OutputTask queue.
		sync_queue<MatchList> out_q;

		// Set up the globber.
		Globber g(ap.m_paths, tm, dim, ap.m_recurse, q);

		// Set up the output task object.
		OutputTask output_task(ap.m_color, ap.m_nocolor, ap.m_column, out_q);

		// Create the FileScanner object.
		FileScanner fs(q, out_q, ap.m_pattern, ap.m_ignore_case, ap.m_word_regexp, ap.m_pattern_is_literal);

		// Start the output task thread.
		std::thread ot {&OutputTask::Run, &output_task};

		// Start the scanner threads.
		for(int t=0; t<ap.m_jobs; ++t)
		{
			std::thread fst {&FileScanner::Run, &fs};
			scanner_threads.push_back(std::move(fst));
		}

		// Start the globber thread last.
		// We do this last because the globber thread is the source; all other threads will be
		// waiting for it to start sending data to the Globber->FileScanner queue.  If we started it
		// first, the globbing would start immediately, and it would take longer to get the scanner and output
		// threads created and started, and ultimately slow down startup.
		std::thread gt {&Globber::Run, &g};

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

		// Check for errors.
		if(g.Error())
		{
			std::cout << "ucg: \"" << g.ErrorPath() << "\": No such file or directory" << std::endl;
			// Both ack and ag return 1 in this situation, which indicates that "no matches were found".
			// We'll follow their lead; this is really sort of an error, and grep would return 2 here,
			// but I suppose it could be argued that there is no match here.
			return 1;
		}

		auto total_matched_lines = output_task.GetTotalMatchedLines();

		if(total_matched_lines == 0)
		{
			// No matches, return a grep-compatible 1.
			return 1;
		}
		else
		{
			// Found some matches, return success.
			return 0;
		}
	}
	catch(const FileScannerException &e)
	{
		std::cerr << "ucg: Error during regex parsing: " << e.what() << std::endl;
		return 255;
	}
	catch(const ArgParseException &e)
	{
		std::cerr << "ucg: Error during arg parsing: " << e.what() << std::endl;
		return 255;
	}
}
