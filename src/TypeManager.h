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

/*
 *
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
	bool FileShouldBeScanned(const std::string &name);

	void CompileTypeTables();

private:

	/// Map of file types to the associated filename extensions.
	std::map<std::string, std::vector<std::string>> m_type_to_extension_map;

	/// File extensions which will be examined.  Maps to file type.
	std::unordered_multimap<std::string, std::string> m_include_extensions;

	/// Literal filenames which will be examined.  Maps to file type.
	std::unordered_multimap<std::string, std::string> m_included_literal_filenames;

	/// Map of the regexes to try to match to the first line of the file (key) to
	/// the file type (value).
	std::unordered_multimap<std::string, std::string> m_included_first_line_regexes;
};

#endif /* TYPEMANAGER_H_ */
