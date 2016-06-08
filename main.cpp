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
#include <cstdlib> // For abort().

#include "sync_queue_impl_selector.h"
#include "Logger.h"
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
		// First thing, set up logging.
		Logger::Init(argv[0]);

		// We'll keep the scanner threads in this vector so we can join() them later.
		std::vector<std::thread> scanner_threads;

		// Instantiate classes for file and directory inclusion/exclusion management.
		TypeManager type_manager;
		DirInclusionManager dir_inclusion_manager;

		// Instantiate the argument parser.
		ArgParse arg_parser(type_manager);

		// Parse command-line options and args.
		arg_parser.Parse(argc, argv);

		dir_inclusion_manager.AddExclusions(arg_parser.m_excludes);

		type_manager.CompileTypeTables();
		dir_inclusion_manager.CompileExclusionTables();

		LOG(INFO) << "Num jobs: " << arg_parser.m_jobs;

		// Create the Globber->FileScanner queue.
		sync_queue<std::string> files_to_scan_queue;

		// Create the FileScanner->OutputTask queue.
		sync_queue<MatchList> match_queue;

		// Set up the globber.
		Globber globber(arg_parser.m_paths, type_manager, dir_inclusion_manager, arg_parser.m_recurse, arg_parser.m_dirjobs, files_to_scan_queue);

		// Set up the output task object.
		OutputTask output_task(arg_parser.m_color, arg_parser.m_nocolor, arg_parser.m_column, match_queue);

		// Create the FileScanner object.
		std::unique_ptr<FileScanner> file_scanner(FileScanner::Create(files_to_scan_queue, match_queue, arg_parser.m_pattern, arg_parser.m_ignore_case, arg_parser.m_word_regexp, arg_parser.m_pattern_is_literal));

		// Start the output task thread.
		std::thread output_task_thread {&OutputTask::Run, &output_task};

		// Start the scanner threads.
		for(int t=0; t<arg_parser.m_jobs; ++t)
		{
			std::thread fst {&FileScanner::Run, file_scanner.get(), t};
			scanner_threads.push_back(std::move(fst));
		}

		// Start the globber thread last.
		// We do this last because the globber thread is the source; all other threads will be
		// waiting for it to start sending data to the Globber->FileScanner queue.  If we started it
		// first, the globbing would start immediately, and it would take longer to get the scanner and output
		// threads created and started, and ultimately slow down startup.
		std::thread globber_thread {&Globber::Run, &globber};

		// Wait for the Globber thread (the source) to finish.
		if(globber_thread.joinable())
			globber_thread.join();

		// Close the Globber->FileScanner queue.
		files_to_scan_queue.close();

		// Wait for all scanner threads to complete.
		for (auto& scanner_thread_ref : scanner_threads)
		{
			scanner_thread_ref.join();
		}
		// All scanner threads completed.

		// Close the FileScanner->OutputTask queue.
		match_queue.close();

		// Wait for the output thread to complete.
		output_task_thread.join();

		// Check for errors.
		if(globber.Error())
		{
			std::cout << "ucg: \"" << globber.ErrorPath() << "\": No such file or directory" << std::endl;
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
		ERROR() << "Error during regex parsing: " << e.what();
		return 255;
	}
	catch(const ArgParseException &e)
	{
		ERROR() << "Error during arg parsing: " << e.what();
		return 255;
	}
	catch(const std::runtime_error &e)
	{
		ERROR() << "std::runtime_error exception: " << e.what();
		return 255;
	}
	catch(...)
	{
		// We shouldn't need this catch(...), but Cygwin seems to not properly call the default
		// terminate handler (which should be abort()) if we let an exception escape main(), but will simply return
		// without an error code.  I ran into this when trying to instantiate std::locale with locale=="" in ArgParse,
		// and before I had the std::runtime_error catch clause above.
		ERROR() << "Unknown exception occurred.";
		std::abort();
	}
}
