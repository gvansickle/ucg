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

/*
 *
 */
class DirTree
{
public:
	/// Type of the file and directory include/exclude predicates.
	using file_basename_filter_type = std::function<bool (const std::string& name) noexcept>;
	using dir_basename_filter_type = std::function<bool (const std::string& name) noexcept>;

	DirTree(sync_queue<FileID>& output_queue,
			const file_basename_filter_type &file_basename_filter,
			const dir_basename_filter_type &dir_basename_filter);
	~DirTree();

	void Scandir(std::vector<std::string> start_paths);

private:

	/// Flag indicating whether we should traverse symlinks or not.
	bool m_logical {true};

	int m_dirjobs {2}; ///@todo

	/// Directory queue.  Used internally.
	sync_queue<std::shared_ptr<FileID>> m_dir_queue;

	/// File output queue.
	sync_queue<FileID>& m_out_queue;

	file_basename_filter_type m_file_basename_filter;
	dir_basename_filter_type m_dir_basename_filter;

	using visited_set = std::unordered_set<dev_ino_pair>;
	std::mutex m_dir_mutex;
	visited_set m_dir_has_been_visited;
	bool HasDirBeenVisited(dev_ino_pair di)
	{
		std::unique_lock<std::mutex> lock(m_dir_mutex);
		return !m_dir_has_been_visited.insert(di).second;
	}

	void ReaddirLoop(int dirjob_num);

	/**
	 * Process a single directory entry (dirent) structure #de, with parent #dse.  Push any files found on the #m_out_queue,
	 * push any directories found on the #m_dir_queue.
	 *
	 * @param dse
	 * @param de
	 */
	void ProcessDirent(std::shared_ptr<FileID> dse, struct dirent *de);

};

#endif /* SRC_LIBEXT_DIRTREE_H_ */
