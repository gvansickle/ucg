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


Globber::Globber(std::vector<std::string> start_paths,
		TypeManager &type_manager,
		DirInclusionManager &dir_inc_manager,
		bool recurse_subdirs,
		int dirjobs,
		sync_queue<std::string>& out_queue)
		: m_start_paths(start_paths),
		  m_type_manager(type_manager),
		  m_dir_inc_manager(dir_inc_manager),
		  m_recurse_subdirs(recurse_subdirs),
		  m_dirjobs(dirjobs),
		  m_out_queue(out_queue)
{

}

Globber::~Globber()
{

}

void Globber::Run()
{
	char * dirs[m_start_paths.size()+1];

	/// @todo It looks like OSX needs any trailing slashes to be removed here, or its fts lib will double them up.
	/// Doesn't seem to affect results though.

	int i = 0;
	for(const std::string& path : m_start_paths)
	{
		dirs[i] = const_cast<char*>(path.c_str());

		// Check if this start path exists and is a file or directory.
		DIR *d = opendir(dirs[i]);
		int f = open(dirs[i], O_RDONLY);

		if((d==NULL) && (f==-1))
		{
			m_bad_path = dirs[i];
		}

		// Close the dir/file we opened.
		if(d != NULL)
		{
			closedir(d);
		}
		if(f != -1)
		{
			close(f);
		}

		if(!m_bad_path.empty())
		{
			// If we couldn't open the specified file/dir, we don't start the globbing, but ultimately
			// return to main() and exit with an error.
			return;
		}

		++i;
	}
	dirs[m_start_paths.size()] = 0;

	sync_queue<std::string> dir_queue;
	std::vector<std::thread> threads;


	// Start the directory traversal threads.  They will all initially block on dir_queue, since it's empty.
	for(int i=0; i<m_dirjobs; i++)
	{
		threads.push_back(std::thread(&Globber::RunSubdirScan, this, std::ref(dir_queue), i));
	}

	LOG(INFO) << "Globber threads = " << threads.size();

	// Push the initial paths to the queue to start the threads off.
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
	// Local copy of a stats struct that we'll use to collect up stats just for this thread.
	DirectoryTraversalStats stats;

	// Set the name of the thread.
	std::stringstream temp_ss;
	temp_ss << "GLOBBER_";
	temp_ss << thread_index;
	set_thread_name(temp_ss.str());

	while(dir_queue.wait_pull(std::move(dir)) != queue_op_status::closed)
	{
		/// The number of directories pushed onto the work queue by this thread during this iteration.
		size_t num_dirs_pushed {0};

		dirs[0] = const_cast<char*>(dir.c_str());
		dirs[1] = 0;

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

			if(ftsent->fts_info == FTS_F || ftsent->fts_info == FTS_D)
			{
				name.assign(ftsent->fts_name, ftsent->fts_namelen);
				path.assign(ftsent->fts_path, ftsent->fts_pathlen);
			}

			LOG(INFO) << "Considering file: " << ftsent->fts_path;
			if(ftsent->fts_info == FTS_F)
			{
				// It's a normal file.
				LOG(INFO) << "... normal file.";
				stats.m_num_files_found++;

				// Check for inclusion.
				if(m_type_manager.FileShouldBeScanned(name))
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
					fts_set(fts, ftsent, FTS_SKIP);
				}
				if(m_dir_inc_manager.DirShouldBeExcluded(path, name))
				{
					// This name is in the dir exclude list.  Exclude the dir and all subdirs from the scan.
					LOG(INFO) << "... should be ignored.";
					fts_set(fts, ftsent, FTS_SKIP);
				}

				if(m_recurse_subdirs && ftsent->fts_level > FTS_ROOTLEVEL && m_dirjobs > 1)
				{
					// Queue it up for scanning.
					dir_queue.wait_push(std::move(path));
					num_dirs_pushed++;
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
				m_bad_path = ftsent->fts_path;
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
				WARN() << "Directory causes cycles: \'" << ftsent->fts_path << "\'";
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
	}

	/// @todo Add the local stats to the class's stats.
}
