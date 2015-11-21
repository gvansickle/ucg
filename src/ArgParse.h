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

#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include <string>
#include <vector>
#include <set>

class TypeManager;

/**
 * Command-line parser.
 */
class ArgParse
{
public:
	ArgParse(TypeManager &tm);
	virtual ~ArgParse();

	void Parse(int argc, char **argv);

public:  /// @todo This should really be private, but we'll need to make parse_opt a member so it has access first.

	/// Reference to the TypeManager passed into the constructor.
	TypeManager &m_type_manager;

public:
	bool m_ignore_case;
	std::string m_pattern;

	/// The file and directory paths given on the command line.
	std::vector<std::string> m_paths;

	/// Directory names to be excluded from the search.
	std::set<std::string> m_excludes;

	/// Number of FileScanner threads to use.
	int m_jobs;

	/// Whether to use color output or not.
	bool m_color;

	/// Whether to recurse into subdirectories or not.
	bool m_recurse;
};

#endif /* ARGPARSE_H_ */
