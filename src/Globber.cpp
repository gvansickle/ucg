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

#include <config.h>
#include <future/string.hpp>
#include <future/string_view.hpp>

#include "Globber.h"

#include <libext/DirTree.h>

#include "TypeManager.h"
#include "DirInclusionManager.h"

#include <libext/Logger.h>

#include <string>


Globber::Globber(std::vector<std::string> start_paths,
		TypeManager &type_manager,
		DirInclusionManager &dir_inc_manager,
		bool recurse_subdirs,
		bool follow_symlinks,
		int dirjobs,
		sync_queue<std::shared_ptr<FileID>>& out_queue)
		: m_start_paths(start_paths),
		  m_type_manager(type_manager),
		  m_dir_inc_manager(dir_inc_manager),
		  m_recurse_subdirs(recurse_subdirs),
		  m_follow_symlinks(follow_symlinks),
		  m_dirjobs(dirjobs),
		  m_out_queue(out_queue)
{

}

void Globber::Run()
{
	auto file_basename_filter = [this](const std::string &basename) noexcept { return m_type_manager.FileShouldBeScanned(basename); };
	auto dir_basename_filter = [this](const std::string &basename) noexcept { return m_dir_inc_manager.DirShouldBeExcluded(basename); };

	DirTree dt(m_out_queue, file_basename_filter, dir_basename_filter, m_recurse_subdirs, m_follow_symlinks);

	dt.Scandir(m_start_paths, m_dirjobs);
}
