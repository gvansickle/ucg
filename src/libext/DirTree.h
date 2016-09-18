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

/// @todo Break this dependency on the output queue class.
#include "../sync_queue_impl_selector.h"
#include "FileID.h"

/*
 *
 */
class DirTree
{
public:
	DirTree(sync_queue<FileID>& output_queue);
	~DirTree();

	/// Type of the file include/exclude predicate.
	using file_name_filter_type = std::function<bool (const std::string& name)>;

	void Read(std::vector<std::string> start_paths, file_name_filter_type &fi);

private:

	sync_queue<FileID>& m_out_queue;
};

#endif /* SRC_LIBEXT_DIRTREE_H_ */
