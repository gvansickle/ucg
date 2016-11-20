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

#ifndef SRC_LIBEXT_DIRTREE_H_
#define SRC_LIBEXT_DIRTREE_H_

#include <config.h>

#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

/// @todo Break this dependency on the output queue class.
#include "../sync_queue_impl_selector.h"
#include "FileID.h"

#include <dirent.h>

/**
 * Helper class to collect up and communicate directory tree traversal stats.
 * The idea is that each thread will maintain its own instance of this class,
 * and only when that thread is complete will it add its statistics to a single, "global"
 * instance.  This avoids any locking concerns other than at that final, one-time sum.
 */
class DirTraversalStats
{
	/**
	 * Using X-macros to make fields easier to add/rearrange/remove.
	 */
#define M_STATLIST \
	X("Number of directories found", m_num_directories_found) \
	X("Number of directories rejected", m_num_dirs_rejected) \
	X("Number of files found", m_num_files_found) \
	X("Number of files rejected", m_num_files_rejected) \
	X("Number of files sent for scanning", m_num_files_scanned) \
	X("Number of files which required a stat() call to determine type", m_num_filetype_stats) \
	X("Number of files which did not require a stat() call to determine type", m_num_filetype_without_stat)

public:
#define X(d,s) size_t s {0};
	M_STATLIST
#undef X

	/**
	 * Atomic compound assignment by sum.
	 * Adds the stats from #other to *this in a thread-safe manner.
	 * @param other
	 */
	void operator+=(const DirTraversalStats & other)
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
	friend std::ostream& operator<<(std::ostream& os, const DirTraversalStats &dts)
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

#undef M_STATLIST
};

/**
 * Directory tree traversal class.
 */
class DirTree
{
public:
	/// Type of the file and directory include/exclude predicates.
	using filter_string_type = std::string;
	using file_basename_filter_type = std::function<bool (const filter_string_type& name) noexcept>;
	using dir_basename_filter_type = std::function<bool (const std::string& name) noexcept>;

	DirTree(sync_queue<FileID>& output_queue,
			const file_basename_filter_type &file_basename_filter,
			const dir_basename_filter_type &dir_basename_filter);
	~DirTree();

	void Scandir(std::vector<std::string> start_paths, int dirjobs);

private:

	/// Flag indicating whether we should traverse symlinks or not.
	bool m_logical {true};

	int m_dirjobs {2};

	/// Directory queue.  Used internally.
	sync_queue<std::shared_ptr<FileID>> m_dir_queue;

	/// File output queue.
	sync_queue<FileID>& m_out_queue;

	file_basename_filter_type m_file_basename_filter;
	dir_basename_filter_type m_dir_basename_filter;

	DirTraversalStats m_stats;

	using visited_set = std::unordered_set<dev_ino_pair>;
	std::mutex m_dir_mutex;
	visited_set m_dir_has_been_visited;
	bool HasDirBeenVisited(dev_ino_pair di)
	{
		std::lock_guard<std::mutex> lock(m_dir_mutex);
		return !m_dir_has_been_visited.insert(di).second;
	}

	void ReaddirLoop(int dirjob_num);

	/**
	 * Process a single directory entry (dirent) structure #de, with parent #dse.  Push any files found on the #m_out_queue,
	 * push any directories found on the #m_dir_queue.  Maintain statistics in #stats.
	 *
	 * @param dse
	 * @param de
	 */
	void ProcessDirent(std::shared_ptr<FileID> dse, struct dirent *de, DirTraversalStats &stats);

};

#endif /* SRC_LIBEXT_DIRTREE_H_ */
