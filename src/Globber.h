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
#include <deque>
#include <string>
#include <thread>
#include <future>

#include "sync_queue_impl_selector.h"

// Forward decls.
class TypeManager;
class DirInclusionManager;

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

	void RunSubdirScan(sync_queue<std::string> &dir_queue);

	std::vector<std::string> m_start_paths;

	TypeManager &m_type_manager;

	DirInclusionManager &m_dir_inc_manager;

	bool m_recurse_subdirs;

	int m_dirjobs;

	sync_queue<std::string>& m_out_queue;

	long m_num_files_found = {0};

	std::string m_bad_path;
};

#endif /* GLOBBER_H_ */
