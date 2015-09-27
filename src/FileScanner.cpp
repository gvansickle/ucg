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
#include "MatchList.h"

#include "config.h"

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
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
		bool ignore_case) : m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex), m_ignore_case(ignore_case),
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

	// The regex we're looking for, possibly ignoring case.
	std::regex expression(m_regex,
			std::regex_constants::ECMAScript | static_cast<typeof(std::regex_constants::icase)>(std::regex_constants::icase * m_ignore_case));

	std::string next_string;
	while(m_in_queue.wait_pull(next_string) != queue_op_status::closed)
	{
		MatchList ml(next_string);

		// Open the file.
		int fd = open(next_string.c_str(), O_RDONLY, 0);

		if(fd == -1)
		{
			// Couldn't open the file, skip it.
			std::cerr << "ERROR: Couldn't open file \"" << next_string << "\"" << std::endl;
			continue;
		}

		// Check the file size.
		struct stat st;
		fstat(fd, &st);
		size_t file_size = st.st_size;
		// If filesize is 0, skip.
		if(file_size == 0)
		{
			std::cerr << "WARNING: Filesize of \"" << next_string << "\" is 0" << std::endl;
			close(fd);
			continue;
		}

		// Read or mmap the file into memory.
		const char *file_data = GetFile(fd, file_size);

		if(file_data == MAP_FAILED)
		{
			// Mapping failed.
			std::cerr << "ERROR: Couldn't map file \"" << next_string << "\"" << std::endl;
			continue;
		}

		// Scan the mmapped file for the regex.
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

		// Clean up.
		FreeFile(file_data, file_size);
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

const char* FileScanner::GetFile(int file_descriptor, size_t file_size)
{
	const char *file_data = static_cast<const char *>(MAP_FAILED);

	if(m_use_mmap)
	{
		file_data = static_cast<const char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, file_descriptor, 0));

		if(file_data == MAP_FAILED)
		{
			// Mapping failed.
			close(file_descriptor);
			return file_data;
		}

		// Hint that we'll be sequentially reading the mmapped file soon.
		posix_madvise(const_cast<char*>(file_data), file_size, POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED);

	}
	else
	{
		file_data = new char [file_size];

		// Read in the whole file.
		read(file_descriptor, const_cast<char*>(file_data), file_size);
	}

	// We don't need the file descriptor anymore.
	close(file_descriptor);

	return file_data;
}

void FileScanner::FreeFile(const char* file_data, size_t file_size)
{
	if(m_use_mmap)
	{
		munmap(const_cast<char*>(file_data), file_size);
	}
	else
	{
		delete [] file_data;
	}
}
