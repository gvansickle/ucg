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

#ifndef TYPEMANAGER_H_
#define TYPEMANAGER_H_

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

/**
 * Class which manages the file types which are to be scanned.
 */
class TypeManager
{
public:
	TypeManager();
	virtual ~TypeManager();

	/**
	 * Determine if the file with the given path and name should be scanned based on the
	 * enabled file types.
	 *
	 * @param path
	 * @param name
	 * @return
	 */
	bool FileShouldBeScanned(const std::string &name) const;

	/**
	 * Add the given file type to the types which will be scanned.  For handling the
	 * --type= command line param.  The first time this function is called, all currently-
	 * active types are removed, and only the given type (and types given subsequently)
	 * will be searched.
	 *
	 * @param type_name  Name of the type.
	 * @return true on success, false if no such type.
	 */
	bool type(const std::string &type_name);

	bool notype(const std::string &type_name);

	void CompileTypeTables();

private:

	/// Flag to keep track of the first call to type().
	bool m_first_type_has_been_seen = { false };

	std::map<std::string, std::vector<std::string>> m_builtin_type_map;

	/// Map of file types to the associated filename extensions.
	/// This is the active map, which will eventually be compiled into the
	/// hash tables used at filename-scanning time.
	std::map<std::string, std::vector<std::string>> m_active_type_map;

	/// File extensions which will be examined.  Maps to file type.
	std::unordered_multimap<std::string, std::string> m_include_extensions;

	/// Literal filenames which will be examined.  Maps to file type.
	std::unordered_multimap<std::string, std::string> m_included_literal_filenames;

	/// Map of the regexes to try to match to the first line of the file (key) to
	/// the file type (value).
	std::unordered_multimap<std::string, std::string> m_included_first_line_regexes;
};

#endif /* TYPEMANAGER_H_ */
