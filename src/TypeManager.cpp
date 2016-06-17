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

#include <config.h>

#include "TypeManager.h"

#include "Logger.h"

#include <algorithm>
#include <set>
#include <iostream>
#include <iomanip>
#include <string>
#include <libext/string.hpp>
#include <fnmatch.h>

struct Type
{
	/// The name of the type.
	std::string m_type_name;

	/// Vector of the extensions, literal strings, and first-line regexes which match the type.
	std::vector<std::string> m_type_extensions;

	/// less-than operator, so that Types are sortable by key (m_type_name).
	bool operator<(const Type &other) const { return m_type_name < other.m_type_name; };
};

static const std::set<Type> f_builtin_type_array =
{
	{ "actionscript", {".as", ".mxml"} },
	{ "ada", {".ada", ".adb", ".ads"} },
	{ "asm", {".asm", ".s", ".S"} },
	{ "asp", {".asp"} },
	{ "aspx", {".master", ".ascx", ".asmx", ".aspx", ".svc"} },
	{ "autoconf", {".ac", ".in"} },
	{ "automake", {".am", ".in"} },
	{ "batch", {".bat", ".cmd"} },
	{ "cc", {".c", ".h", ".xs"} },
	{ "cfmx", {".cfc", ".cfm", ".cfml"} },
	{ "clojure", {".clj"} },
	{ "cmake", {"CMakeLists.txt", ".cmake"} },
	{ "coffeescript", {".coffee"} },
	{ "cpp", {".cpp", ".cc", ".cxx", ".m", ".hpp", ".hh", ".h", ".hxx"} },
	{ "csharp", {".cs"} },
	{ "css", {".css"} },
	{ "dart", {".dart"} },
	{ "delphi", {".pas", ".int", ".dfm", ".nfm", ".dof", ".dpk", ".dproj", ".groupproj", ".bdsgroup", ".bdsproj"} },
	{ "elisp", {".el"} },
	{ "elixir", {".ex", ".exs"} },
	{ "erlang", {".erl", ".hrl"} },
	{ "fortran", {".f", ".f77", ".f90", ".f95", ".f03", ".for", ".ftn", ".fpp"} },
	{ "go", {".go"} },
	{ "groovy", {".groovy", ".gtmpl", ".gpp", ".grunit", ".gradle"} },
	{ "haskell", {".hs", ".lhs"} },
	{ "hh", {".h"} },
	{ "html", {".htm", ".html"} },
	{ "jade", {".jade"} },
	{ "java", {".java", ".properties"} },
	{ "js", {".js"} },
	{ "json", {".json"} },
	{ "jsp", {".jsp", ".jspx", ".jhtm", ".jhtml"} },
	{ "less", {".less"} },
	{ "lisp", {".lisp", ".lsp"} },
	{ "lua", {".lua", R"(/^#!.*\blua(jit)?/)"} },
	{ "m4", {".m4"} },
	{ "make", {".mk", ".mak", "makefile", "Makefile", "Makefile.Debug", "Makefile.Release"} },
	{ "matlab", {".m"} },
	{ "objc", {".m", ".h"} },
	{ "objcpp", {".mm", ".h"} },
	{ "ocaml", {".ml", ".mli"} },
	{ "parrot", {".pir", ".pasm", ".pmc", ".ops", ".pod", ".pg", ".tg"} },
	{ "perl", {".pl", ".pm", ".pod", ".t", ".psgi", R"(/^#!.*\bperl/)"} },
	{ "perltest", {".t"} },
	{ "php", {".php", ".phpt", ".php3", ".php4", ".php5", ".phtml", R"(/^#!.*\bphp/)"} },
	{ "plone", {".pt", ".cpt", ".metadata", ".cpy", ".py"} },
	{ "python", {".py", R"(/^#!.*\bpython/)"} },
	{ "rake", {"Rakefile"} },
	{ "rr", {".R"} },
	{ "rst", {".rst"} },
	{ "ruby", {".rb", ".rhtml", ".rjs", ".rxml", ".erb", ".rake", ".spec", "Rakefile", R"(/^#!.*\bruby/)"} },
	{ "rust", {".rs"} },
	{ "sass", {".sass", ".scss"} },
	{ "scala", {".scala"} },
	{ "scheme", {".scm", ".ss"} },
	{ "shell", {".sh", ".bash", ".csh", ".tcsh", ".ksh", ".zsh", ".fish", R"(/^#!.*\b(?:ba|t?c|k|z|fi)?sh\b/)"} },
	{ "smalltalk", {".st"} },
	{ "smarty", {".tpl"} },
	{ "sql", {".sql", ".ctl"} },
	{ "stylus", {".styl"} },
	{ "tcl", {".tcl", ".itcl", ".itk"} },
	{ "tex", {".tex", ".cls", ".sty"} },
	{ "text", {".txt", "ChangeLog", "README"} },
	{ "tt", {".tt", ".tt2", ".ttml"} },
	{ "vb", {".bas", ".cls", ".frm", ".ctl", ".vb", ".resx"} },
	{ "verilog", {".v", ".vh", ".sv"} },
	{ "vhdl", {".vhd", ".vhdl"} },
	{ "vim", {".vim"} },
	{ "xml", {".xml", ".dtd", ".xsl", ".xslt", ".ent", R"(/<[?]xml/)"} },
	{ "yaml", {".yaml", ".yml"} },
	// Below here are types corresponding to some of the files ack 2.14 finds as non-binary by scanning them.
	// We'll do that at some point too, but for now just include them here.
	{ "miscellaneous", {".qbk", ".w", ".ipp", ".patch", "configure"} }
};


TypeManager::TypeManager()
{
	// Populate the type map with the built-in defaults.
	for(auto t : f_builtin_type_array)
	{
		m_builtin_and_user_type_map[t.m_type_name] = t.m_type_extensions;
		m_active_type_map[t.m_type_name] = t.m_type_extensions;
	}
}

bool TypeManager::FileShouldBeScanned(const std::string& name) const noexcept
{
	// Find the name's extension.
	auto last_period_offset = name.find_last_of('.');
	auto last_period = last_period_offset == std::string::npos ? name.cend() : name.cbegin() + last_period_offset;
	if(last_period != name.cend())
	{
		// There was a period, might be an extension.
		if(last_period != name.cbegin())
		{
			bool include_it = false;

			// Name doesn't start with a period, it still could be an extension.
			auto ext_plus_period_size = name.cend() - last_period;

			if(ext_plus_period_size <= 5)
			{
				// Use the 4-byte fast map.
				microstring mext(last_period+1, name.cend());
				include_it = std::binary_search(m_fast_include_extensions.cbegin(), m_fast_include_extensions.cend(), mext);
			}
			else if(m_include_extensions.find(std::string(last_period, name.cend())) != m_include_extensions.end())
			{
				// Found the extension in the hash of extensions to include.
				include_it = true;
			}

			// Now check that a glob pattern doesn't subsequently exclude it.
			if(include_it && IsExcludedByAnyGlob(name))
			{
				return false;
			}

			return include_it;
		}
	}

	// Check if the filename is one of the literal filenames we're supposed to look at.
	if(m_included_literal_filenames.find(name) != m_included_literal_filenames.end())
	{
		// It matches a literal filename, but now check that a glob pattern doesn't subsequently exclude it.
		if(IsExcludedByAnyGlob(name))
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	// Now the checks start to get expensive.  Check if the filename matches any of the globbing patterns
	// we're including or excluding.
	for(auto glob : m_include_exclude_globs)
	{
		int result = fnmatch(glob.first.c_str(), name.c_str(), 0);
		if(result == 0)
		{
			// Glob matched, return whether we include or exclude.
			return glob.second;
		}
	}

	/// @todo Support first-line regexes.

	return false;
}

bool TypeManager::type(const std::string& type_name)
{
	auto it_type = m_builtin_and_user_type_map.find(type_name);

	if(it_type == m_builtin_and_user_type_map.end())
	{
		// No such type currently is defined.
		return false;
	}

	if(!m_first_type_has_been_seen)
	{
		// This is the first call to type(), clear the active Type map.
		m_active_type_map.clear();
	}
	m_first_type_has_been_seen = true;

	// Remove the filters from the removed-filters map, if they have been added.
	/// @note Ack doesn't appear to do this.  If you give it a command line such as:
	/// ack --noenv --type=nocpp --type=nocc --type=hh '#endif' ~/src/boost_1_58_0
	/// you'll get no hits even though there are .h files in the directory.
	/// Ag doesn't appear to support more than one type spec at a time, an no --noTYPEs, so
	/// doesn't run into this issue.  I think the correct behavior here is to 'un-remove'
	/// any removed filters in this situation, such that:
	/// ucg --noenv --type=nocpp --type=nocc --type=hh '#endif' ~/src/boost_1_58_0
	/// and
	/// ucg --noenv --type=hh '#endif' ~/src/boost_1_58_0
	/// give the same results.
	for(auto i : it_type->second)
	{
		m_removed_type_filters.erase(i);
	}

	// Add the type to the active type map.
	m_active_type_map.insert(*it_type);

	return true;
}

bool TypeManager::notype(const std::string& type_name)
{
	auto it_type = m_builtin_and_user_type_map.find(type_name);

	if(it_type == m_builtin_and_user_type_map.end())
	{
		// No such type currently is defined.
		return false;
	}

	// Add the filters to the removed-filters map.
	for(auto i : it_type->second)
	{
		m_removed_type_filters.insert(std::make_pair(i, type_name));
	}

	// Remove the type from the active type map.
	m_active_type_map.erase(type_name);

	return true;
}

bool TypeManager::IsType(const std::string& type) const
{
	return m_builtin_and_user_type_map.count(type) != 0;
}

void TypeManager::TypeAddFromFilterSpecString(bool delete_type_first, const std::string& filter_spec_string)
{
	std::string file_type;
	std::string filter_type;
	std::string filter_args;

	auto filter_spec = split(filter_spec_string, ':');

	if(filter_spec.size() != 3)
	{
		// We didn't get the full 3-part spec.  Fail.
		throw TypeManagerException("Invalid filter specification \"" + filter_spec_string + "\"");
	}

	file_type = filter_spec[0];
	filter_type = filter_spec[1];
	filter_args = filter_spec[2];

	if(delete_type_first)
	{
		// This is a --type-set, so delete any existing file type first.
		TypeDel(file_type);
	}

	// Figure out which filter type was specified and do the appropriate TypeAddXxx.
	if(filter_type == "is")
	{
		// filter_args is a literal filename.
		TypeAddIs(file_type, filter_args);
	}
	else if(filter_type == "ext")
	{
		// filter_args is a list of one or more comma-separated filename extensions.
		auto exts = split(filter_args, ',');
		for(auto ext : exts)
		{
			TypeAddExt(file_type, ext);
		}
	}
	else if(filter_type == "glob")
	{
		TypeAddGlob(file_type, filter_args);
	}
	else
	{
		// Unsupported or unknown filter type.
		throw TypeManagerException("Unknown filter type \"" + filter_type + "\" in type spec \"" + filter_spec_string + "\"");
	}
}


void TypeManager::TypeAddIgnoreFileFromFilterSpecString(const std::string& filter_spec_string)
{
	// Use this special file type for all --ignore-file= file type specs.
	std::string file_type_name {"IGNORE_FILE_TYPE"};

	// Add the filter spec to the special file type.
	TypeAddFromFilterSpecString(false, file_type_name + ":" + filter_spec_string);

	// --notype= the filter spec.
	notype(file_type_name);
}

void TypeManager::TypeAddIs(const std::string& type, const std::string& name)
{
	m_builtin_and_user_type_map[type].push_back(name);
	m_active_type_map[type].push_back(name);
}

void TypeManager::TypeAddExt(const std::string& type, const std::string& ext)
{
	m_builtin_and_user_type_map[type].push_back("."+ext);
	m_active_type_map[type].push_back("."+ext);
}


void TypeManager::TypeAddGlob(const std::string& type, const std::string& glob)
{
	m_builtin_and_user_type_map[type].push_back("?"+glob);
	m_active_type_map[type].push_back("?"+glob);
}

bool TypeManager::TypeDel(const std::string& type)
{
	m_active_type_map.erase(type);
	auto num_erased = m_builtin_and_user_type_map.erase(type);

	return num_erased > 0;
}

bool TypeManager::IsExcludedByAnyGlob(const std::string &name) const noexcept
{
	for(auto glob : m_include_exclude_globs)
	{
		int result = fnmatch(glob.first.c_str(), name.c_str(), 0);
		if(result == 0)
		{
			// Glob matched, check if we should exclude.
			if(glob.second == false)
			{
				return true;
			}
		}
	}

	return false;
}

void TypeManager::CompileTypeTables()
{
	std::set<microstring> unique_4char_extensions;

	for(auto i : m_active_type_map)
	{
		for(auto j : i.second)
		{
			// First check if this filter spec has been removed by a call to notype().
			// Remember that more than one type can contain the same filter spec.
			if(m_removed_type_filters.count(j) != 0)
			{
				// It has, skip the insertion into the file-search-time maps.
				continue;
			}

			// Determine the filter type and put it in the correct map.
			if(j[0] == '.')
			{
				// First char is a '.', this is an extension specification.
				if(j.length() <= 5)
				{
					// It's 4 chars or less, minus the '.'.
					microstring m(j.begin()+1, j.end());
					unique_4char_extensions.insert(m);
				}
				else
				{
					m_include_extensions.emplace(j, i.first);
				}
			}
			else if(j[0] == '/')
			{
				// First char is a '/', it's a first-line regex.
				m_included_first_line_regexes.emplace(j, i.first);
			}
			else if(j[0] == '?')
			{
				// First char is a '?', it's a glob-exclude pattern.
				m_include_exclude_globs.emplace_back(std::make_pair(j.substr(1), false));
			}
			else
			{
				// It's a literal filename (e.g. "Makefile").
				m_included_literal_filenames.emplace(j, i.first);
			}
		}
	}

	m_fast_include_extensions.resize(unique_4char_extensions.size());
	LOG(INFO) << "Found " << unique_4char_extensions.size() << " unique 4-char or less extensions.";
	decltype(m_fast_include_extensions)::size_type j = 0;
	for(auto i : unique_4char_extensions)
	{
		m_fast_include_extensions[j] = i;
		++j;

		LOG(INFO) << "Added " << static_cast<std::string>(i) << "(" << std::hex << static_cast<std::string>(i) << ") to m_fast_include_extensions";
	}

	/// @todo shouldn't need this.
	std::sort(m_fast_include_extensions.begin(), m_fast_include_extensions.end());
}

void TypeManager::PrintTypesForHelp(std::ostream& s) const
{
	for(auto t : m_builtin_and_user_type_map)
	{
		s << "  " << std::setw(15) << std::left << t.first;

		std::string extensions, names;
		for(auto e : t.second)
		{
			if(e[0] == '.')
			{
				// It's an extension.
				if(extensions.empty())
				{
					extensions += e;
				}
				else
				{
					extensions += " " + e;
				}
			}
			else if(e[0] == '/')
			{
				/// @todo First-line regex, currently not supported.
			}
			else
			{
				// It's a literal filename.
				if(names.empty())
				{
					names += e;
				}
				else
				{
					names += " " + e;
				}
			}
		}
		s << extensions;
		if(!extensions.empty() && !names.empty())
		{
			s << "; ";
		}
		s << names;
		s << std::endl;
	}
}

