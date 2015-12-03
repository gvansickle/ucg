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

#include "FileScanner.h"
#include "File.h"
#include "MatchList.h"

#include "config.h"

#include <iostream>
#include <regex>
#include <thread>
#include <mutex>
#ifndef HAVE_SCHED_SETAFFINITY
#else
	#include <sched.h>
#endif

static std::mutex f_assign_affinity_mutex;

FileScanner::FileScanner(sync_queue<std::string> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp) : m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex),
				m_ignore_case(ignore_case), m_word_regexp(word_regexp),
				m_next_core(0), m_use_mmap(false), m_manually_assign_cores(false)
{

}

FileScanner::~FileScanner()
{
	// TODO Auto-generated destructor stub
}

void FileScanner::Run()
{
	if(m_manually_assign_cores)
	{
		// Spread the scanner threads across cores.  Linux at least doesn't seem to want to do that by default.
		AssignToNextCore();
	}

	// The regex we're looking for, possibly ignoring case, possibly with match-whole-word.
	auto stack_regex = m_regex;
	if(m_word_regexp)
	{
		// Surround the regex with \b (word boundary) assertions.
		stack_regex = "\\b(?:" + m_regex + ")\\b";
	}
	std::regex expression(stack_regex,
			std::regex_constants::ECMAScript |
			std::regex_constants::optimize   |
			static_cast<typeof(std::regex_constants::icase)>(std::regex_constants::icase * m_ignore_case));

	// Pull new filenames off the input queue until it's closed.
	std::string next_string;
	while(m_in_queue.wait_pull(next_string) != queue_op_status::closed)
	{
		MatchList ml(next_string);

		try
		{
			// Try to open and read the file.  This could throw.
			File f(next_string);

			if(f.size() == 0)
			{
				std::cerr << "WARNING: Filesize of \"" << next_string << "\" is 0" << std::endl;
				continue;
			}

			const char *file_data = f.data();
			size_t file_size = f.size();

			// Scan the file for the regex.
			std::regex_iterator<const char *> rit(file_data, file_data+file_size, expression);
			std::regex_iterator<const char *> rend;
			while(rit != rend)
			{
				//std::cout << "Match in file " << next_string << std::endl;

				long long lineno = 1+std::count(file_data, file_data+rit->position(), '\n');
				auto line_ending = "\n";
				auto line_start = std::find_end(file_data, file_data+rit->position(),
						line_ending, line_ending+1);
				if(line_start == file_data+rit->position())
				{
					// The line has no starting '\n', so it must be the first line.
					line_start = file_data;
				}
				else
				{
					// The line had a starting '\n', clip it off.
					++line_start;
				}
				auto line_end = std::find(file_data+rit->position(), file_data+file_size, '\n');
				auto pre_match = std::string(line_start, file_data+rit->position());
				auto match = std::string(rit->begin()->str());
				auto post_match = std::string(file_data+rit->position()+rit->length(), line_end);
				Match m = { pre_match, match, post_match };
				ml.AddMatch(lineno, m);

				++rit;
			}

			if(!ml.empty())
			{
				/// @todo Move semantics here?
				m_output_queue.wait_push(ml);
			}
		}
		catch(const std::system_error& error)
		{
			// A system error.  Currently should only be errors from File.
			std::cerr << "Error: " << error.code() << " - " << error.code().message() << std::endl;
		}
		catch(...)
		{
			// Rethrow whatever it was.
			throw;
		}
	}
}

void FileScanner::AssignToNextCore()
{
#ifdef HAVE_SCHED_SETAFFINITY

	// Prevent the multiple threads from stepping on each other and screwing up m_next_core.
	std::lock_guard<std::mutex> lg {f_assign_affinity_mutex};

	cpu_set_t cpuset;

	// Clear the cpu_set_t.
	CPU_ZERO(&cpuset);

	// Set the bit of the next CPU.
	CPU_SET(m_next_core, &cpuset);

	sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	// Increment so we use the next core the next time.
	m_next_core++;
	m_next_core %= std::thread::hardware_concurrency();
#endif
}
