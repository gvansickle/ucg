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

#ifndef DIRINCLUSIONMANAGER_H_
#define DIRINCLUSIONMANAGER_H_

#include <config.h>

#include <string>
#include <set>

/**
 * Class for managing the inclusion and exclusion of directories from the search.
 */
class DirInclusionManager
{
public:
	DirInclusionManager();
	~DirInclusionManager();

	void AddExclusions(const std::set<std::string> &exclusions);

	void CompileExclusionTables();

	/**
	 * Returns true if:
	 * - The @a name param (i.e. <...>/dir) matches one of the literal strings
	 *   in m_excluded_literal_dirs.
	 *
	 * @param path
	 * @return
	 */
	[[nodiscard]] bool DirShouldBeExcluded(const std::string &name) const;

private:

	/// Literal directory names (not containing '/') which will be excluded.
	std::set<std::string> m_excluded_literal_dirs;
};

#endif /* DIRINCLUSIONMANAGER_H_ */
