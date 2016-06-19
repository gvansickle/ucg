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

#include <vector>
#include <set>
#include <string>
#include <thread>
#include <sys/types.h> // for dev_t, ino_t

#include <libext/integer.hpp>

#include "sync_queue_impl_selector.h"

// Forward decls.
class TypeManager;
class DirInclusionManager;

/**
 * Helper struct to collect up and communicate traversal stats.
 */
struct DirectoryTraversalStats
{
	size_t m_num_files_found { 0 };

	size_t m_num_files_rejected { 0 };

	size_t m_num_files_scanned { 0 };

	size_t m_num_directories_found { 0 };
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
			sync_queue<std::string> &out_queue);
	virtual ~Globber();

	void Run();

	bool Error() const noexcept { return m_bad_path.size() != 0; };

	std::string ErrorPath() const noexcept { return m_bad_path; };

private:

	void RunSubdirScan(sync_queue<std::string> &dir_queue, int thread_index);

	std::vector<std::string> m_start_paths;

	TypeManager &m_type_manager;

	DirInclusionManager &m_dir_inc_manager;

	bool m_recurse_subdirs;

	int m_dirjobs;

	sync_queue<std::string>& m_out_queue;

	using dev_ino_pair_type = uint_t<(sizeof(dev_t)+sizeof(ino_t))*8>::fast;
	struct dev_ino_pair
	{
		dev_ino_pair(dev_t d, ino_t i) noexcept { m_val = d, m_val <<= sizeof(ino_t)*8, m_val |= i; };

		dev_ino_pair_type m_val;
	};

	std::mutex m_dir_mutex;
	std::set<dev_ino_pair_type> m_dir_has_been_visited;
	bool HasDirBeenVisited(dev_ino_pair_type di) { std::unique_lock<std::mutex> lock(m_dir_mutex); return !m_dir_has_been_visited.insert(di).second; };

	DirectoryTraversalStats m_traversal_stats;

	std::string m_bad_path;
};


#endif /* GLOBBER_H_ */
