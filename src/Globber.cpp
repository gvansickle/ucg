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

#include <config.h>
#include <future/string.hpp>
#include <future/string_view.hpp>

#include "Globber.h"

#include <libext/DirTree.h>


#include <iomanip>
#include "TypeManager.h"
#include "DirInclusionManager.h"

#include <fts.h>
#include <dirent.h>
#include <fcntl.h>
#include <libext/Logger.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <utility>
#include <system_error>
#include <string>
#include <cstring>
#include <algorithm>
#include <libext/string.hpp>

/// @todo FOR TEST, DELETE
#define USE_DIRTREE 1


Globber::Globber(std::vector<std::string> start_paths,
		TypeManager &type_manager,
		DirInclusionManager &dir_inc_manager,
		bool recurse_subdirs,
		int dirjobs,
		sync_queue<FileID>& out_queue)
		: m_start_paths(start_paths),
		  m_num_start_paths_remaining(start_paths.size()),
		  m_type_manager(type_manager),
		  m_dir_inc_manager(dir_inc_manager),
		  m_recurse_subdirs(recurse_subdirs),
		  m_dirjobs(dirjobs),
		  m_out_queue(out_queue)
{

}

void Globber::Run()
{
#if USE_DIRTREE == 1 /// @todo TEMP

	auto file_basename_filter = [this](const DirTree::filter_string_type &basename) noexcept { return m_type_manager.FileShouldBeScanned(basename); };
	auto dir_basename_filter = [this](const std::string &basename) noexcept { return m_dir_inc_manager.DirShouldBeExcluded(basename); };

	DirTree dt(m_out_queue, file_basename_filter, dir_basename_filter);

	dt.Scandir(m_start_paths, m_dirjobs);

	return;
#else

	sync_queue<DirQueueEntry> dir_queue;

	/// @todo It looks like OSX needs any trailing slashes to be removed from the m_start_paths here, or its fts lib will double them up.
	/// Doesn't seem to affect the overall scanning results though.

	//
	// Step 1: Process the paths and/or filenames specified by the user on the command line.
	// We always use only a single thread (the current one) for this step.
	//

	// Push the initial paths to the queue.
	LOG(INFO) << "Number of start paths = " << m_start_paths.size();
	for(auto path : m_start_paths)
	{
		DirQueueEntry dqe;
		dqe.m_pathname = path;
		dqe.m_level = FTS_ROOTLEVEL;
		dir_queue.wait_push(std::move(dqe));
	}
	// Process those files/directories.  This call will push the root-level directories onto the dir_queue,
	// and the files onto the m_out_queue.  It will not loop and pull more dirs off the dir_queue and keep processing.
	RunSubdirScan(dir_queue, 9999);

	if(dir_queue.size() > 0)
	{
		// Create and start the directory traversal threads.
		std::vector<std::thread> threads;

		for(int i=0; i<m_dirjobs; i++)
		{
			threads.push_back(std::thread(&Globber::RunSubdirScan, this, std::ref(dir_queue), i));
		}

		LOG(INFO) << "Globber threads = " << threads.size();

		// Wait for the producer+consumer threads to finish.
		dir_queue.wait_for_worker_completion(m_dirjobs);

		dir_queue.close();

		// Wait for all the threads to finish.
		for(auto &thr : threads)
		{
			thr.join();
		}
	}
	else
	{
		LOG(INFO) << "No paths below root level to process.";
		dir_queue.close();
	}
#endif
	// Log the traversal stats.
	LOG(INFO) << m_traversal_stats;

}
