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

#include "Globber.h"

#include "Logger.h"
#include "TypeManager.h"
#include "DirInclusionManager.h"

#include <fts.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <utility>
#include <system_error>
#include <string>
#include <libext/string.hpp>


Globber::Globber(std::vector<std::string> start_paths,
		TypeManager &type_manager,
		DirInclusionManager &dir_inc_manager,
		bool recurse_subdirs,
		int dirjobs,
		sync_queue<std::string>& out_queue)
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
	sync_queue<std::string> dir_queue;
	std::vector<std::thread> threads;

	/// @todo It looks like OSX needs any trailing slashes to be removed from the m_start_paths here, or its fts lib will double them up.
	/// Doesn't seem to affect the overall scanning results though.

	// Start the directory traversal threads.  They will all initially block on dir_queue, since it's empty.
	for(int i=0; i<m_dirjobs; i++)
	{
		threads.push_back(std::thread(&Globber::RunSubdirScan, this, std::ref(dir_queue), i));
	}

	LOG(INFO) << "Globber threads = " << threads.size();

	// Push the initial paths to the queue to start the threads off.
	LOG(INFO) << "Number of start paths = " << m_start_paths.size();
	for(auto path : m_start_paths)
	{
		dir_queue.wait_push(path);
	}

	// Wait for the producer+consumer threads to finish.
	dir_queue.wait_for_worker_completion(m_dirjobs);

	dir_queue.close();

	// Wait for all the threads to finish.
	for(auto &thr : threads)
	{
		thr.join();
	}

	///LOG(INFO) << "Number of regular files found: " << m_num_files_found;
}


void Globber::RunSubdirScan(sync_queue<std::string> &dir_queue, int thread_index)
{
	char * dirs[2];
	std::string dir;

	// Local for optimizing the determination of whether the paths specified on the command line have been consumed.
	bool start_paths_have_been_consumed = false;

	// Local copy of a stats struct that we'll use to collect up stats just for this thread.
	DirectoryTraversalStats stats;

	// Set the name of the thread.
	set_thread_name("GLOBBER_" + std::to_string(thread_index));

	while(dir_queue.wait_pull(std::move(dir)) != queue_op_status::closed)
	{
		dirs[0] = const_cast<char*>(dir.c_str());
		dirs[1] = 0;
		size_t old_val {0};

		// If we haven't seen m_num_start_paths_remaining == 0 yet...
		if(!start_paths_have_been_consumed)
		{
			// Compare-and-Exchange loop to decrement the m_num_start_paths_remaining counter by 1.
			size_t old_val = m_num_start_paths_remaining.load();
			while(old_val != 0 && !m_num_start_paths_remaining.compare_exchange_weak(old_val, old_val - 1))
			{
				// Spin until we get a successful compare and exchange.  If old_val was already 0, we're done decrementing,
				// so skip the spin here entirely.
			}
		}

		/// @todo We can't use FS_NOSTAT here.  OSX at least isn't able to determine regular
		/// files without the stat, so they get returned as FTS_NSOK / 11 /	no stat(2) requested.
		/// Does not seem to affect performance on Linux, but might be having an effect on Cygwin.
		/// Look into workarounds.
		/// @note Per looking at the fts_open() source, FTS_LOGICAL turns on FTS_NOCHDIR, but since we're traversing
		/// in multiple threads, and there's only a process-wide cwd, we'll specify it anyway.
		/// @todo Current gnulib supports additional flags here: FTS_CWDFD | FTS_DEFER_STAT | FTS_NOATIME.  We should
		/// check for these and use them if they exist.
		FTS *fts = fts_open(dirs, FTS_LOGICAL | FTS_NOCHDIR /*| FTS_NOSTAT*/, NULL);
		while(FTSENT *ftsent = fts_read(fts))
		{
			std::string name;
			std::string path;
			bool skip_inclusion_checks = false;

			if(ftsent->fts_info == FTS_F || ftsent->fts_info == FTS_D)
			{
				name.assign(ftsent->fts_name, ftsent->fts_namelen);
				path.assign(ftsent->fts_path, ftsent->fts_pathlen);
			}

			LOG(INFO) << "Considering file \'" << ftsent->fts_path << "\' at depth = " << ftsent->fts_level;

			// Determine if we should skip the inclusion/exclusion checks for this file/dir.  We should only do this
			// for files/dirs specified on the command line, which will have an fts_level of FTS_ROOTLEVEL (0), and the
			// start_paths_have_been_consumed flag will still be false.
			if((ftsent->fts_level == FTS_ROOTLEVEL) && !start_paths_have_been_consumed)
			{
				skip_inclusion_checks = true;
			}

			if(ftsent->fts_info == FTS_F)
			{
				// It's a normal file.
				LOG(INFO) << "... normal file.";
				stats.m_num_files_found++;

				// Check for inclusion.
				if(skip_inclusion_checks || m_type_manager.FileShouldBeScanned(name))
				{
					LOG(INFO) << "... should be scanned.";
					// Extension was in the hash table.
					m_out_queue.wait_push(std::move(path));

					// Count the number of files we found that were included in the search.
					stats.m_num_files_scanned++;
				}
				else
				{
					stats.m_num_files_rejected++;
				}
			}
			else if(ftsent->fts_info == FTS_D)
			{
				LOG(INFO) << "... directory.";
				stats.m_num_directories_found++;
				// It's a directory.  Check if we should descend into it.
				if(!m_recurse_subdirs && ftsent->fts_level > FTS_ROOTLEVEL)
				{
					// We were told not to recurse into subdirectories.
					LOG(INFO) << "... --no-recurse specified, skipping.";
					fts_set(fts, ftsent, FTS_SKIP);
				}
				if(!skip_inclusion_checks && m_dir_inc_manager.DirShouldBeExcluded(path, name))
				{
					// This name is in the dir exclude list.  Exclude the dir and all subdirs from the scan.
					LOG(INFO) << "... should be ignored.";
					fts_set(fts, ftsent, FTS_SKIP);
				}

				if((m_dirjobs > 1) && (ftsent->fts_level == FTS_ROOTLEVEL))
				{
					// We're doing the directory traversal multithreaded, so we have to detect cycles ourselves.
					if(HasDirBeenVisited(dev_ino_pair(ftsent->fts_dev, ftsent->fts_ino).m_val))
					{
						// Found cycle.
						WARN() << "\'" << ftsent->fts_path << "\': recursive directory loop";
						fts_set(fts, ftsent, FTS_SKIP);
						continue;
					}
				}

				if(m_recurse_subdirs && (ftsent->fts_level > FTS_ROOTLEVEL) && (m_dirjobs > 1))
				{
					// Queue it up for scanning.
					LOG(INFO) << "... subdir, queuing it up for multithreaded scanning.";
					dir_queue.wait_push(std::move(path));
					fts_set(fts, ftsent, FTS_SKIP);
				}
			}
			/// @note Only FTS_DNR, FTS_ERR, and FTS_NS have valid fts_errno information.
			else if(ftsent->fts_info == FTS_DNR)
			{
				// A directory that couldn't be read.
				NOTICE() << "Unable to read directory \'" << ftsent->fts_path << "\': "
						<< LOG_STRERROR(ftsent->fts_errno) << ". Skipping.";
			}
			else if(ftsent->fts_info == FTS_ERR)
			{
				ERROR() << "Directory traversal error at path \'" << ftsent->fts_path << "\': "
						<< LOG_STRERROR(ftsent->fts_errno) << ".";
				break;
			}
			else if(ftsent->fts_info == FTS_NS)
			{
				// No stat info.
				NOTICE() << "Could not get stat info at path \'" << ftsent->fts_path << "\': "
									<< LOG_STRERROR(ftsent->fts_errno) << ". Skipping.";
			}
			else if(ftsent->fts_info == FTS_DC)
			{
				// Directory that causes cycles.
				WARN() << "\'" << ftsent->fts_path << "\': recursive directory loop";
			}
			else if(ftsent->fts_info == FTS_SLNONE)
			{
				// Broken symlink.
				WARN() << "Broken symlink: \'" << ftsent->fts_path << "\'";
			}
			else
			{
				LOG(INFO) << "... unknown file type:" << ftsent->fts_info;
			}
		}
		fts_close(fts);

		if(old_val == 0)
		{
			// We've consumed all of the paths given on the command line, we no longer need to do the check.
			LOG(INFO) << "All start paths consumed.";
			start_paths_have_been_consumed = true;
		}

	}

	/// @todo Add the local stats to the class's stats.
}
