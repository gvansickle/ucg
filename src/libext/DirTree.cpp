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

#include "Logger.h"

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

/// Estimate that we'll traverse no more than 10000 directories in one traversal.
/// m_dir_has_been_visited will resize/rehash if it needs more space.
constexpr auto M_INITIAL_NUM_DIR_ESTIMATE = 10000;

DirTree::DirTree(sync_queue<std::shared_ptr<FileID>>& output_queue,
		const file_basename_filter_type &file_basename_filter,
		const dir_basename_filter_type &dir_basename_filter,
		bool follow_symlinks)
	: m_follow_symlinks(follow_symlinks), m_out_queue(output_queue),
	  m_file_basename_filter(file_basename_filter), m_dir_basename_filter(dir_basename_filter)
{
	m_dir_has_been_visited.reserve(M_INITIAL_NUM_DIR_ESTIMATE);
}

DirTree::~DirTree()
{
}

void DirTree::Scandir(std::vector<std::string> start_paths, int dirjobs)
{
	m_dirjobs = dirjobs;

	// Start at the cwd of the process (~AT_FDCWD)
	std::shared_ptr<FileID> root_file_id = std::make_shared<FileID>(FileID::path_known_cwd_tag());

	//
	// Step 1: Process the paths and/or filenames specified by the user on the command line.
	// We always use only a single thread (the current one) for this step.
	//

	for(auto p : start_paths)
	{
		// Clean up the paths coming from the command line.
		p = clean_up_path(p);

		/// @note At the moment, we're doing the equivalent of fts' COMFOLLOW here;
		/// we follow symlinks during the fstatat() call in the FileID constructor by not specifying
		/// AT_SYMLINK_NOFOLLOW.  So, we shouldn't get a FT_SYMLINK back from GetFileType().
		auto file_or_dir = std::make_shared<FileID>(FileID(root_file_id, p));
		auto type = file_or_dir->GetFileType();
		switch(type)
		{
		case FT_REG:
		{
			// Explicitly not filtering files specified on command line.
			file_or_dir->SetFileDescriptorMode(FAM_RDONLY, FCF_NOATIME | FCF_NOCTTY);
			m_out_queue.wait_push(file_or_dir);
			break;
		}
		case FT_DIR:
		{
			file_or_dir->SetFileDescriptorMode(FAM_RDONLY, FCF_DIRECTORY | FCF_NOATIME | FCF_NOCTTY | FCF_NONBLOCK);
			m_dir_queue.wait_push(file_or_dir);
			break;
		}
		case FT_SYMLINK:
		{
			// Should never get this.
			ERROR() << "Got filetype of symlink while following symlinks";
			break;
		}
		case FT_STAT_FAILED:
		{
			// Couldn't get any info on this path.
			NOTICE() << "Could not get stat info at path \'" << file_or_dir->GetPath() << "\': "
												<< LOG_STRERROR(errno) << ". Skipping.";
			break;
		}
		default:
		{
			// Ignore all other types.
			NOTICE() << "Unsupported file type at path \'" << file_or_dir->GetPath() << "\': " << ". Skipping.";
			break;
		}
		}
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
	LOG(INFO) << "FileID stats:\n" << *root_file_id;
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
		LOG(DEBUG) << "Examining files in directory '" << dse->GetPath() << "'";

		// Get a DIR* representing the directory specified by dse.
		d = dse->OpenDir();
		if(d == nullptr)
		{
			// At a minimum, this wasn't a directory.
			WARN() << "OpenDir() failed on path " << dse->GetBasename() << ": " << LOG_STRERROR();
			continue;
		}

		do
		{
			if((dp = readdir(d)) != NULL)
			{
				ProcessDirent(dse, dp, stats);
			}
		} while(dp != NULL);

		// Check if readdir is just done with this directory, or encountered an error.
		if(errno != 0)
		{
			WARN() << "Could not read directory: " << LOG_STRERROR(errno) << ". Skipping.";
			errno = 0;
		}

		dse->CloseDir(d);
	}

	m_stats += stats;
}


void DirTree::ProcessDirent(std::shared_ptr<FileID> dse, struct dirent* current_dirent, DirTraversalStats &stats)
{
	struct stat statbuf;

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
		stats.m_num_filetype_without_stat++;
		return;
	}
	if(!is_unknown)
	{
		// We know the type from the dirent.
		stats.m_num_filetype_without_stat++;
	}
#endif
	const char *dname = current_dirent->d_name;
	// Skip "." and "..".
	if(dname[0] == '.' && (dname[1] == 0 || (dname[1] == '.' && dname[2] == 0)))
	{
		// Always skip "." and "..", unless they're specified on the command line.
		stats.m_num_dotdirs_found++;
		stats.m_num_dotdirs_rejected++;
		stats.m_num_filetype_without_stat++;
		return;
	}

	struct stat *statbuff_ptr = nullptr;

	if((is_unknown) || (m_follow_symlinks && is_symlink))
	{
		// We now have one of two situations:
		// - The dirent didn't know what the type of the file was, or
		// - The dirent told us this was a symlink, and we're doing a logical traversal.
		//   Now we have to actually stat this entry and see what it is.
		//   Note that if the situation is m_logical+is_symlink, we want to find out
		//   where it goes, so we follow the symlink.

		stats.m_num_filetype_stats++;

		// Stat the filename using the directory as the at-descriptor.
		dse->FStatAt(dname, &statbuf, AT_NO_AUTOMOUNT | (!m_follow_symlinks ? AT_SYMLINK_NOFOLLOW : 0));

		is_dir = S_ISDIR(statbuf.st_mode);
		is_file = S_ISREG(statbuf.st_mode);
		/// @note This shouldn't ever come back as a symlink if we're doing a logical traversal, since
		///       fstatat() follows symlinks by default.  We add the AT_SYMLINK_NOFOLLOW flag and then
		///       ignore any symlinks returned if we're doing a physical traversal.
		is_symlink = S_ISLNK(statbuf.st_mode);
		if(is_dir || is_file || is_symlink)
		{
			is_unknown = false;
		}

		// Capture the stat info we just got for the FileID object we'll create below.
		statbuff_ptr = &statbuf;
	}

	// Is the file type still unknown?
	if(is_unknown)
	{
		WARN() << "cannot determine file type: " << dname << ", " << statbuf.st_mode;
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
			if(m_file_basename_filter(basename))
			{
				// Based on the file name, this file should be scanned.

				LOG(INFO) << "... should be scanned.";

				std::shared_ptr<FileID> file_to_scan = std::make_shared<FileID>(FileID::path_known_relative_tag(), dse, basename, statbuff_ptr, FT_REG);
				if(statbuff_ptr == nullptr)
				{
					file_to_scan->SetDevIno(dse->GetDev(), current_dirent->d_ino);
				}
				file_to_scan->SetFileDescriptorMode(FAM_RDONLY, FCF_NOCTTY | FCF_NOATIME);

				// Queue it up.
				m_out_queue.wait_push(std::move(file_to_scan));

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

			if(m_dir_basename_filter(basename))
			{
				// This name is in the dir exclude list.  Exclude the dir and all subdirs from the scan.
				LOG(INFO) << "... should be ignored.";
				stats.m_num_dirs_rejected++;
				return;
			}

			auto dir_atfd = std::make_shared<FileID>(FileID::path_known_relative_tag(), dse, basename, statbuff_ptr, FT_DIR);
			if(statbuff_ptr == nullptr)
			{
				dir_atfd->SetDevIno(dse->GetDev(), current_dirent->d_ino);
			}
			dir_atfd->SetFileDescriptorMode(FAM_RDONLY, FCF_DIRECTORY | FCF_NOATIME | FCF_NOCTTY | FCF_NONBLOCK);

			if(m_follow_symlinks)
			{
				// We have to detect any symlink cycles ourselves.
				if(HasDirBeenVisited(dir_atfd->GetUniqueFileIdentifier()))
				{
					// Found cycle.
					WARN() << "'" << dir_atfd->GetPath() << "': already visited this directory, possible recursive directory loop?";
					stats.m_num_dirs_rejected++;
					return;
				}
			}

			m_dir_queue.wait_push(std::move(dir_atfd));
		}
		else if(is_symlink)
		{
			if(m_follow_symlinks)
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
