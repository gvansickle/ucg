/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file  */

#include <config.h>

#include "DirTree.h"

#include "../Logger.h" ///< @todo Break this root-ward dependency.

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>

#include <stdexcept>
#include <iostream>
#include <queue>
#include <map>
#include <algorithm>

#include <future/memory.hpp>
#include <libext/filesystem.hpp> // For AT_FDCWD, AT_NO_AUTOMOUNT, openat(), etc.


DirTree::DirTree(sync_queue<FileID>& output_queue,
		const file_basename_filter_type &file_basename_filter,
		const dir_basename_filter_type &dir_basename_filter)
	: m_out_queue(output_queue), m_file_basename_filter(file_basename_filter), m_dir_basename_filter(dir_basename_filter)
{
}

DirTree::~DirTree()
{
}

void DirTree::Scandir(std::vector<std::string> start_paths, int dirjobs)
{
	m_dirjobs = dirjobs;

	// Start at the cwd of the process (~AT_FDCWD)
	std::shared_ptr<FileID> root_file_id = std::make_shared<FileID>(FileID::path_known_cwd_tag());

	for(auto p : start_paths)
	{
		// Clean up the incoming paths.
		p = realpath(p);

		auto file_or_dir = std::make_shared<FileID>(FileID(root_file_id, p));
		auto type = file_or_dir->GetFileType();
		if(type == FT_REG)
		{
			/// @todo filter-out mechanism?
			m_out_queue.wait_push(FileID(std::move(*file_or_dir)));
		}
		else if(type == FT_DIR)
		{
			m_dir_queue.wait_push(file_or_dir);
		}
		else if(type == FT_STAT_FAILED)
		{
			// Couldn't get any info on this path.
			NOTICE() << "Could not get stat info at path \'" << file_or_dir->GetPath() << "\': "
												<< LOG_STRERROR(errno) << ". Skipping.";
		}
		/// @todo Symlinks, COMFOLLOW.
	}

	// Create and start the directory traversal threads.
	std::vector<std::thread> threads;

	for(int i=0; i<m_dirjobs; i++)
	{
		threads.push_back(std::thread(&DirTree::ReaddirLoop, this, i));
	}

	LOG(INFO) << "Globber threads = " << threads.size();

	// Wait for the producer+consumer threads to finish.
	m_dir_queue.wait_for_worker_completion(m_dirjobs);

	m_dir_queue.close();

	// Wait for all the threads to finish.
	for(auto &thr : threads)
	{
		thr.join();
	}

	// Log the traversal stats.
	LOG(INFO) << m_stats;
}

void DirTree::ReaddirLoop(int dirjob_num)
{
	std::shared_ptr<FileID> dse;
	DIR *d {nullptr};
	struct dirent *dp {nullptr};

	DirTraversalStats stats;

	// Set the name of this thread, for logging and debug purposes.
	set_thread_name("READDIR_" + std::to_string(dirjob_num));

	while(m_dir_queue.wait_pull(std::move(dse)) != queue_op_status::closed)
	{
		int open_at_fd = dse->GetAtDir()->GetFileDescriptor().GetFD();
		const char *open_at_path = dse->GetAtDirRelativeBasename().c_str();

		d = opendirat(open_at_fd, open_at_path);
		if(d == nullptr)
		{
			// At a minimum, this wasn't a directory.
			WARN() << "opendirat() failed on path " << open_at_path << ": " << LOG_STRERROR();
			errno = 0;
			continue;
		}

		do
		{
			errno = 0;
			if((dp = readdir(d)) != NULL)
			{
				ProcessDirent(dse, dp, stats);
			}
		} while(dp != NULL);

		if(errno != 0)
		{
			WARN() << "Could not read directory: " << LOG_STRERROR(errno) << ". Skipping.";
			errno = 0;
		}

		closedir(d);
	}

	m_stats += stats;
}


void DirTree::ProcessDirent(std::shared_ptr<FileID> dse, struct dirent* current_dirent, DirTraversalStats &stats)
{
	bool is_dir {false};
	bool is_file {false};
	bool is_symlink {false};
	bool is_unknown {true};

#ifdef _DIRENT_HAVE_D_TYPE
	// Reject anything that isn't a directory, a regular file, or a symlink.
	// If it's DT_UNKNOWN, we'll have to do a stat to find out.
	is_dir = (current_dirent->d_type == DT_DIR);
	is_file = (current_dirent->d_type == DT_REG);
	is_symlink = (current_dirent->d_type == DT_LNK);
	is_unknown = (current_dirent->d_type == DT_UNKNOWN);
	if(!is_file && !is_dir && !is_symlink && !is_unknown)
	{
		// It's a type we don't care about.
		return;
	}
#endif
	const char *dname = current_dirent->d_name;
	// Skip "." and "..".
	if(dname[0] == '.' && (dname[1] == 0 || (dname[1] == '.' && dname[2] == 0)))
	{
		// Always skip "." and "..", unless they're specified on the command line.
		return;
	}

	struct stat statbuf;
	struct stat *statbuff_ptr = nullptr;

	if((is_unknown) || (m_logical && is_symlink))
	{
		// We now have one of two situations:
		// - The dirent didn't know what the type of the file was, or
		// - The dirent told us this was a symlink, and we're doing a logical traversal.
		//   Now we have to actually stat this entry and see what it is.
		//   Note that if the situation is m_logical+is_symlink, we want to find out
		//   where it goes, so we follow the symlink.

		// Stat the filename using the directory as the at-descriptor.
		int retval = fstatat(dse->GetFileDescriptor().GetFD(), dname, &statbuf,
				AT_NO_AUTOMOUNT | (!m_logical ? AT_SYMLINK_NOFOLLOW : 0));
		if(retval == -1)
		{
			WARN() << "Attempt to stat file '" << dname << "' in directory '" << dse->GetPath() << "' failed: " << LOG_STRERROR();
			errno = 0;
			return;
		}
		/// @todo Capture this info in the FileID object.
		is_dir = S_ISDIR(statbuf.st_mode);
		is_file = S_ISREG(statbuf.st_mode);
		/// @todo This shouldn't ever come back as a symlink if we're doing a logical traversal, since
		///       fstatat() follows symlinks by default.  We add the AT_SYMLINK_NOFOLLOW flag and then
		///       ignore any symlinks returned if we're doing a physical traversal.
		is_symlink = S_ISLNK(statbuf.st_mode);
		if(is_dir || is_file || is_symlink)
		{
			is_unknown = false;
		}
		statbuff_ptr = &statbuf;
	}

	// Is the file type still unknown?
	if(is_unknown)
	{
		WARN() << "cannot determine file type: " << dname << ", " << statbuf.st_mode << '\n';
		return;
	}

	// We now know the type for certain.
	// Is this a file type we're interested in?
	if(is_file || is_dir || is_symlink)
	{
		// We'll need the file's basename.
		std::string basename = dirent_get_name(current_dirent);

		LOG(INFO) << "Considering dirent name='" << basename << "'";

		if(is_file)
		{
			// It's a normal file.
			LOG(INFO) << "... normal file.";
			stats.m_num_files_found++;

			// Check for inclusion.
			if(m_file_basename_filter(basename)) //skip_inclusion_checks || m_type_manager.FileShouldBeScanned(name))
			{
				// Based on the file name, this file should be scanned.

				LOG(INFO) << "... should be scanned.";

				m_out_queue.wait_push({FileID::path_known_relative, dse, basename, statbuff_ptr, FT_REG});

				// Count the number of files we found that were included in the search.
				stats.m_num_files_scanned++;
			}
			else
			{
				stats.m_num_files_rejected++;
			}
		}
		else if(is_dir)
		{
			LOG(INFO) << "... directory.";
			stats.m_num_directories_found++;

			if(m_dir_basename_filter(basename)) //!skip_inclusion_checks && m_dir_inc_manager.DirShouldBeExcluded(name))
			{
				// This name is in the dir exclude list.  Exclude the dir and all subdirs from the scan.
				LOG(INFO) << "... should be ignored.";
				stats.m_num_dirs_rejected++;
				return;
			}

			FileID dir_atfd(FileID::path_known_relative, dse, basename, FT_DIR);
			dir_atfd.SetDevIno(dse->GetDev(), current_dirent->d_ino);

			if(m_logical)
			{
				// We have to detect any symlink cycles ourselves.
				if(HasDirBeenVisited(dir_atfd.GetUniqueFileIdentifier()))
				{
					// Found cycle.
					WARN() << "'" << dir_atfd.GetPath() << "': recursive directory loop";
					stats.m_num_dirs_rejected++;
					return;
				}
			}

			m_dir_queue.wait_push(std::make_shared<FileID>(std::move(dir_atfd)));
		}
		else if(is_symlink)
		{
			if(m_logical)
			{
				// Logical traversal, should never get here.
				ERROR() << "found unresolved symlink during logical traversal";
			}
			else
			{
				// Physical traversal, just ignore the symlink.
				LOG(INFO) << "Found symlink during physical traversal: '" << dse->GetPath() << "/" << basename << "'";
			}
			return;
		}
	}
}
