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
 * The idea is that each thread will maintain its own instance of this class,
 * and only when that thread is complete will it add its statistics to a single, "global"
 * instance.  This avoids any locking concerns other than at that final, one-time sum.
 */
class DirectoryTraversalStats
{
#define M_STATLIST \
	X("Number of directories found", m_num_directories_found) \
	X("Number of directories rejected", m_num_dirs_rejected) \
	X("Number of files found", m_num_files_found) \
	X("Number of files rejected", m_num_files_rejected) \
	X("Number of files sent for scanning", m_num_files_scanned)

public:
#define X(d,s) size_t s {0};
				M_STATLIST
#undef X

	/**
	 * Atomic compound assignment by sum.
	 * Adds the stats from #other to *this in a thread-safe manner.
	 * @param other
	 */
	void operator+=(const DirectoryTraversalStats & other)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

#define X(d,s) s += other. s;
				M_STATLIST
#undef X
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
		return os
#define X(d,s) << "\n" d ": " << dts. s
				M_STATLIST
#undef X
		;
	};

private:

	/// Mutex for making the compound assignment by sum operator thread-safe.
	std::mutex m_mutex;
};

struct DirQueueEntry
{
	DirQueueEntry() = default;
	DirQueueEntry(FTSENT *ftsent);
	DirQueueEntry(DirQueueEntry&&) = default;
	~DirQueueEntry() = default;

	DirQueueEntry& operator=(DirQueueEntry&&) = default;
	DirQueueEntry& operator=(const DirQueueEntry&) = default;

	std::string m_pathname;
	int64_t m_level {FTS_ROOTPARENTLEVEL};
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

	void RunSubdirScan(sync_queue<DirQueueEntry> &dir_queue, int thread_index);

	void ScanOneDirectory(FTS *tree, sync_queue<DirQueueEntry> &dir_queue, DirectoryTraversalStats &stats);

	/// Vector of the paths which the user gave on the command line.
	std::vector<std::string> m_start_paths;

	std::atomic<size_t> m_num_start_paths_remaining;

	/// Reference to the TypeManager which will be used to include or exclude the files we find during the traversal.
	TypeManager &m_type_manager;

	/// Reference to the DirInclusionManager which will be used to include or exclude the directories we traverse.
	DirInclusionManager &m_dir_inc_manager;

	bool m_recurse_subdirs;

	bool m_logical {true};

	bool m_using_nostat {false};

	int m_dirjobs;

	sync_queue<FileID>& m_out_queue;

	std::mutex m_dir_mutex;
	std::set<dev_ino_pair> m_dir_has_been_visited;
	bool HasDirBeenVisited(dev_ino_pair di) { std::unique_lock<std::mutex> lock(m_dir_mutex); return !m_dir_has_been_visited.insert(di).second; };

	DirectoryTraversalStats m_traversal_stats;
};


#endif /* GLOBBER_H_ */
