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

/// @todo
#include <libext/DirTree.h>


#include "Logger.h"
#include <iomanip>
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
#include <future/string.hpp>
#include <libext/string.hpp>

/// @todo FOR TEST, DELETE
#define TRAVERSE_ONLY 0
#define USE_DIRTREE 0



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
	sync_queue<std::string> dir_queue;
	std::vector<std::thread> threads;

#if USE_DIRTREE == 1 /// @todo TEMP
	DirTree dt;
	dt.Read(m_start_paths);
	return;
#endif

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

	// Log the traversal stats.
	LOG(INFO) << m_traversal_stats;
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
		size_t num_dirs_found_this_loop {0};

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
		/// check for these and use them if they exist.  Note the following though regarding O_NOATIME from the GNU libc
		/// docs <https://www.gnu.org/software/libc/manual/html_node/Operating-Modes.html#Operating-Modes>:
		/// "Only the owner of the file or the superuser may use this bit. This is a GNU extension."
		int fts_options = FTS_COMFOLLOW | (m_using_nostat * FTS_NOSTAT);
		if(m_logical)
		{
			// Do a logical traversal (follow symlinks).  If available, this needs FTS_TIGHT_CYCLE_CHECK.
			fts_options |= FTS_LOGICAL;
#if defined(FTS_TIGHT_CYCLE_CHECK)
			fts_options |= FTS_TIGHT_CYCLE_CHECK;
#endif
		}
		else
		{
			// Do a physical traversal (don't follow symlinks).
			fts_options |= FTS_PHYSICAL;
		}
#if defined(FTS_DEFER_STAT)
		fts_options |= FTS_DEFER_STAT;
#endif
#if defined(FTS_NOATIME)
		// According to GNU libc docs, this is best-effort.  According to Linux docs, it only applies if you're root.
		fts_options |= FTS_NOATIME;
#endif
#if defined(FTS_CWDFD)
		fts_options |= FTS_CWDFD;
#else
		fts_options |= FTS_NOCHDIR;
#endif
		FTS *fts = fts_open(dirs, fts_options, NULL);
		if(fts == nullptr)
		{
			perror("fts error");
			/// @todo Throw here.
		}
		FTSENT *ftsent;
		while((ftsent = fts_read(fts)) != nullptr)
		{
			LOG(INFO) << "fts_read() called with fts->fts_path='" << fts->fts_path << "', returned...";
			if(ftsent == nullptr) {LOG(INFO) << "... ftsent==nullptr"; }
			else {LOG(INFO) << "ftsent->fts_path=" << ftsent->fts_path; };

			// Iterate through all entries in this directory.
			FTSENT *child = fts_children(fts, 0);
			if(errno != 0)
			{
				perror("error calling fts_children()");
			}
			if((child == nullptr) && (errno == 0))
			{
				// The FTSENT most recently returned by fts_read() is not a directory being visited in preorder,
				// or the directory does not contain any files.  Skip it.
				continue;
			}

			while(nullptr != child)
			{
				std::string name;

				bool skip_inclusion_checks = false;

				LOG(INFO) << "Considering file path/name \'" << ftsent_path(child) << " /// " << ftsent_name(child)
						<< "\' at depth = " << child->fts_level;

				// Determine if we should skip the inclusion/exclusion checks for this file/dir.  We should only do this
				// for files/dirs specified on the command line, which will have an fts_level of FTS_ROOTLEVEL (0), and the
				// start_paths_have_been_consumed flag will still be false.
				if((child->fts_level == FTS_ROOTLEVEL) && !start_paths_have_been_consumed)
				{
					skip_inclusion_checks = true;
				}

				switch(child->fts_info)
				{
				case FTS_F:
				{
					// It's a normal file.
					LOG(INFO) << "... normal file.";
					stats.m_num_files_found++;

					// Check for inclusion.
					name.assign(child->fts_name, child->fts_namelen);
					if(skip_inclusion_checks || m_type_manager.FileShouldBeScanned(name))
					{
						// Based on the file name, this file should be scanned.

						LOG(INFO) << "... should be scanned.";

						m_out_queue.wait_push(FileID(child, !m_using_nostat));

						// Count the number of files we found that were included in the search.
						stats.m_num_files_scanned++;
					}
					else
					{
						stats.m_num_files_rejected++;
					}

					break;
				}
				case FTS_D:
				{
					LOG(INFO) << "... directory.";
					stats.m_num_directories_found++;

					// It's a directory.  Check if we should descend into it.
					if(!m_recurse_subdirs && child->fts_level > FTS_ROOTLEVEL)
					{
						// We were told not to recurse into subdirectories.
						LOG(INFO) << "... --no-recurse specified, skipping.";
						fts_set(fts, child, FTS_SKIP);
					}

					// Now we need the name in a std::string.
					name.assign(child->fts_name, child->fts_namelen);

					if(!skip_inclusion_checks && m_dir_inc_manager.DirShouldBeExcluded(name.c_str()))
					{
						// This name is in the dir exclude list.  Exclude the dir and all subdirs from the scan.
						LOG(INFO) << "... should be ignored.";
						stats.m_num_dirs_rejected++;
						fts_set(fts, child, FTS_SKIP);
					}

					// We possibly have some more work to do if we're doing a multithreaded traversal.
					if(m_dirjobs > 1)
					{
						if(m_logical && child->fts_level == FTS_ROOTLEVEL)
						{
							// We're doing the directory traversal multithreaded, so we have to detect cycles ourselves.
							if(HasDirBeenVisited(dev_ino_pair(child->fts_dev, child->fts_ino)))
							{
								// Found cycle.
								WARN() << "\'" << child->fts_path << "\': recursive directory loop";
								fts_set(fts, child, FTS_SKIP);
								continue;
							}
						}
						if(m_recurse_subdirs && (child->fts_level > FTS_ROOTLEVEL))
						{
							if(num_dirs_found_this_loop == 0)
							{
								// We're doing the directory traversal multithreaded, so we have to detect cycles ourselves.
								if(m_logical && HasDirBeenVisited(dev_ino_pair(child->fts_dev, child->fts_ino)))
								{
									// Found cycle.
									WARN() << "\'" << child->fts_path << "\': recursive directory loop";
									fts_set(fts, child, FTS_SKIP);
									continue;
								}

								// Handle this one ourselves.
								LOG(INFO) << "... subdir, not queuing it up for multithreaded scanning, handling it from same FTS stream.";
								num_dirs_found_this_loop++;
							}
							else
							{
								// We're doing the directory traversal multithreaded, so queue it up for scanning.
								LOG(INFO) << "... subdir, queuing it up for multithreaded scanning.";
								dir_queue.wait_push(ftsent_path(child));
								fts_set(fts, child, FTS_SKIP);
								num_dirs_found_this_loop++;
							}
						}
					}

					LOG(INFO) << "Pre-order visit to dir \'" << ftsent_path(child) << "\', setting fts_pointer==" << std::hex << child->fts_pointer;

					break;
				}
				case FTS_DP:
				{
					LOG(INFO) << "Post-order visit to dir \'" << ftsent_path(child) << "\', fts_pointer==" << std::hex << child->fts_pointer;
					break;
				}
				case FTS_NSOK:
				{
					// No stat info was requested because fts_open() was called with FTS_NOSTAT, and we didn't get any.
					// Otherwise, we shouldn't get here.
					NOTICE() << "No stat info requested for \'" << ftsent_path(child) << "\'.  Skipping.";
					break;
				}
				/// @note Only FTS_DNR, FTS_ERR, and FTS_NS have valid fts_errno information.
				case FTS_DNR:
				{
					// A directory that couldn't be read.
					NOTICE() << "Unable to read directory \'" << ftsent_path(child) << "\': "
							<< LOG_STRERROR(child->fts_errno) << ". Skipping.";
					break;
				}
				case FTS_ERR:
				{
					ERROR() << "Directory traversal error at path \'" << ftsent_path(child) << "\': "
							<< LOG_STRERROR(child->fts_errno) << ".";

					/// @todo Break out of loop entirely?
					break;
				}
				case FTS_NS:
				{
					// No stat info.
					NOTICE() << "Could not get stat info at path \'" << ftsent_path(child) << "\': "
										<< LOG_STRERROR(child->fts_errno) << ". Skipping.";
					break;
				}
				case FTS_DC:
				{
					// Directory that causes cycles.
					WARN() << "\'" << child->fts_path << "\': recursive directory loop";
					break;
				}
				case FTS_SLNONE:
				{
					// Broken symlink.
					WARN() << "Broken symlink: \'" << ftsent_path(child) << "\'";
					break;
				}
				default:
				{
					LOG(INFO) << "... unknown file type:" << child->fts_info;
					break;
				}
				}

				if(child->fts_link != nullptr)
				{
					// Go to the next FTSENT in the linked list.
					child = child->fts_link;
				}
				else
				{
					// No more entries, break out of the linked-list loop, and back to the fts_read() loop.
					goto THIS_DIR_COMPLETE;
				}
			}
		}
THIS_DIR_COMPLETE:
		fts_close(fts);

		if(old_val == 0)
		{
			// We've consumed all of the paths given on the command line, we no longer need to do the check.
			LOG(INFO) << "All start paths consumed.";
			start_paths_have_been_consumed = true;
		}
	}

	// Add the local stats to the class's stats.
	m_traversal_stats += stats;
}

void Globber::ScanOneDirectory(FTS *tree, const char *filename, int *flags)
{

}
