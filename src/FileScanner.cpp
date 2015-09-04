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

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <regex>

FileScanner::FileScanner(boost::concurrent::sync_queue<std::string> &in_queue,
		boost::concurrent::sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case) : m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex), m_ignore_case(ignore_case)
{

}

FileScanner::~FileScanner()
{
	// TODO Auto-generated destructor stub
}

void FileScanner::Run()
{
	// The regex we're looking for, possibly ignoring case.
	std::regex expression(m_regex,
			std::regex_constants::ECMAScript | static_cast<typeof(std::regex_constants::icase)>(std::regex_constants::icase * m_ignore_case));

	std::string next_string;
	while(m_in_queue.wait_pull(next_string) != boost::concurrent::queue_op_status::closed)
	{
		///std::cout << "=" << next_string << std::endl;
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
			std::cerr << "WARNING: Filesize is 0" << std::endl;
			close(fd);
			continue;
		}

		// mmap the file into memory.
		const char *file_data = static_cast<const char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, fd, 0));

		if(file_data == MAP_FAILED)
		{
			// Mapping failed.
			std::cerr << "ERROR: Couldn't map file \"" << next_string << "\"" << std::endl;
			close(fd);
			continue;
		}

		// Hint that we'll be sequentially reading the mmapped file soon.
		posix_madvise(const_cast<char*>(file_data), file_size, POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED);

		// We don't need the file descriptor anymore.
		close(fd);

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
		munmap(const_cast<char*>(file_data), file_size);
	}
}

