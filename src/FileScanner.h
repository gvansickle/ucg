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
#include <boost/thread/sync_queue.hpp>

#include "MatchList.h"

/**
 * Class which does the actual regex scanning.
 */
class FileScanner
{
public:
	FileScanner(boost::concurrent::sync_queue<std::string> &in_queue,
			boost::concurrent::sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case);
	virtual ~FileScanner();

	void Run();

private:

	void AssignToNextCore();

	const char* GetFile(int file_descriptor, size_t file_size);
	void FreeFile(const char * file_data, size_t file_size);

	boost::concurrent::sync_queue<std::string>& m_in_queue;

	boost::concurrent::sync_queue<MatchList> &m_output_queue;

	std::string m_regex;

	bool m_ignore_case;

	int m_next_core;

	bool m_use_mmap;
};

#endif /* FILESCANNER_H_ */
