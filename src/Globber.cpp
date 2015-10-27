/*
 * Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "Globber.h"

#include "TypeManager.h"

#include <fts.h>
#include <fnmatch.h>
#include <cstring>
#include <iostream>
#include <utility>


Globber::Globber(std::vector<std::string> start_paths,
		TypeManager &type_manager,
		sync_queue<std::string>& out_queue)
		: m_start_paths(start_paths),
		  m_out_queue(out_queue),
		  m_type_manager(type_manager)
{

}

Globber::~Globber()
{
	// TODO Auto-generated destructor stub
}

void Globber::Run()
{
	char * dirs[m_start_paths.size()+1];

	int i = 0;
	for(auto path : m_start_paths)
	{
		dirs[i] = const_cast<char*>(path.c_str());
		++i;
	}
	dirs[m_start_paths.size()] = 0;

	FTS *fts = fts_open(dirs, FTS_LOGICAL | FTS_NOCHDIR | FTS_NOSTAT, NULL);
	while(FTSENT *ftsent = fts_read(fts))
	{
		if(ftsent->fts_info == FTS_F)
		{
			// It's a normal file.  Check for inclusion.
			if(m_type_manager.FileShouldBeScanned(ftsent->fts_name))
			{
				// Extension was in the hash table.
				m_out_queue.wait_push(std::string(ftsent->fts_path));

				// Count the number of files we found that were included in the search.
				m_num_files_found++;
			}
		}
	}
	fts_close(fts);

	std::clog << "NUM FILES INCLUDED: " << m_num_files_found << std::endl;
}

