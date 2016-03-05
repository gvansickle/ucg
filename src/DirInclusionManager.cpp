/*
 * Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of UltimateCodeSearch.
 *
 * UltimateCodeSearch is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * UltimateCodeSearch is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * UltimateCodeSearch.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file */

#include "DirInclusionManager.h"

/**
 * Default directories which will be ignored.
 */
static const std::string f_builtin_dir_excludes[] =
{
	".bzr",
	".git",
	".hg",
	".metadata",
	".svn",
	"CMakeFiles",
	"CVS",
	"autom4te.cache",
	""
};

DirInclusionManager::DirInclusionManager()
{
	// TODO Auto-generated constructor stub

}

DirInclusionManager::~DirInclusionManager()
{
	// TODO Auto-generated destructor stub
}

void DirInclusionManager::AddExclusions(const std::set<std::string>& exclusions)
{
	for(auto name : exclusions)
	{
		m_excluded_literal_dirs.insert(name);
	}
}

void DirInclusionManager::CompileExclusionTables()
{
	// Populate the exclusion set with the built-in defaults.
	size_t i = 0;
	std::string t;
	while(t = f_builtin_dir_excludes[i], !t.empty())
	{
		m_excluded_literal_dirs.insert(t);
		++i;
	}
}

bool DirInclusionManager::DirShouldBeExcluded(const std::string& /*path*/, const std::string &name) const
{
	/// @todo Commented out path above to avoid unused param warning.  Plan is to eventually use that
	/// for full-path regex filtering, since we get the path for free from fts (see Globber.cpp).

	if(m_excluded_literal_dirs.count(name) != 0)
	{
		// This directory shouldn't be traversed.
		return true;
	}

	// No exclusion rules matched, descend into the directory.
	return false;
}
