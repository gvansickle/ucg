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

#ifndef GLOBBER_H_
#define GLOBBER_H_

#include <config.h>

#include <iostream>

#include <vector>
#include <set>
#include <string>
#include <thread>
#include <atomic>
#include <libext/filesystem.hpp>
#include "sync_queue_impl_selector.h"

#include "FileID.h"

// Forward decls.
class TypeManager;
class DirInclusionManager;

/**
 * Helper class to collect up and communicate directory tree traversal stats.
 */
class DirectoryTraversalStats
{
public:
	size_t m_num_files_found { 0 };
	size_t m_num_directories_found { 0 };
	size_t m_num_files_rejected { 0 };
	size_t m_num_files_scanned { 0 };
	size_t m_num_dirs_rejected { 0 };

	/**
	 * Atomic compound assignment by sum.
	 * Adds the stats from #other to *this in a thread-safe manner.
	 * @param other
	 */
	void operator+=(const DirectoryTraversalStats & other)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_num_files_found += other.m_num_files_found;
		m_num_directories_found += other.m_num_directories_found;
		m_num_files_rejected += other.m_num_files_rejected;
		m_num_files_scanned += other.m_num_files_scanned;
		m_num_dirs_rejected += other.m_num_dirs_rejected;
	}

	/**
	 * Friend function stream insertion operator.
	 *
	 * @param os
	 * @param dts
	 * @return
	 */
	friend std::ostream& operator<<(std::ostream& os, const DirectoryTraversalStats &dts)
	{
		return os << "Number of files found: " << dts.m_num_files_found
				<< "\nNumber of directories found: " << dts.m_num_directories_found
				<< "\nNumber of files rejected: " << dts.m_num_files_rejected
				<< "\nNumber of files sent for scanning: " << dts.m_num_files_scanned
				<< "\nNumber of directories rejected: " << dts.m_num_dirs_rejected;

	};

private:

	/// Mutex for making the compound assignment by sum operator thread-safe.
	std::mutex m_mutex;
};


/**
 * This class does the directory tree traversal.
 */
class Globber
{
public:
	Globber(std::vector<std::string> start_paths,
			TypeManager &type_manager,
			DirInclusionManager &dir_inc_manager,
			bool recurse_subdirs,
			int dirjobs,
			sync_queue<FileID> &out_queue);
	~Globber() = default;

	void Run();

private:

	void RunSubdirScan(sync_queue<std::string> &dir_queue, int thread_index);

	/// Vector of the paths which the user gave on the command line.
	std::vector<std::string> m_start_paths;

	std::atomic<size_t> m_num_start_paths_remaining;

	/// Reference to the TypeManager which will be used to include or exclude the files we find during the traversal.
	TypeManager &m_type_manager;

	/// Reference to the DirInclusionManager which will be used to include or exclude the directories we traverse.
	DirInclusionManager &m_dir_inc_manager;

	bool m_recurse_subdirs;

	bool m_logical {false};

	int m_dirjobs;

	sync_queue<FileID>& m_out_queue;

	std::mutex m_dir_mutex;
	std::set<dev_ino_pair> m_dir_has_been_visited;
	bool HasDirBeenVisited(dev_ino_pair di) { std::unique_lock<std::mutex> lock(m_dir_mutex); return !m_dir_has_been_visited.insert(di).second; };

	DirectoryTraversalStats m_traversal_stats;
};


#endif /* GLOBBER_H_ */
