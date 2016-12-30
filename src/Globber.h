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
#include <string>
#include "libext/FileID.h"
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
			bool follow_symlinks,
			int dirjobs,
			sync_queue<std::shared_ptr<FileID>> &out_queue);
	~Globber() = default;

	void Run();

private:

	/// Vector of the paths which the user gave on the command line.
	std::vector<std::string> m_start_paths;

	/// Reference to the TypeManager which will be used to include or exclude the files we find during the traversal.
	TypeManager &m_type_manager;

	/// Reference to the DirInclusionManager which will be used to include or exclude the directories we traverse.
	DirInclusionManager &m_dir_inc_manager;

	bool m_recurse_subdirs;

	bool m_follow_symlinks;

	int m_dirjobs;

	sync_queue<std::shared_ptr<FileID>>& m_out_queue;
};


#endif /* GLOBBER_H_ */
