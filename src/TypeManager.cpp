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

#include "TypeManager.h"

#include <algorithm>
#include <set>
#include <iomanip>

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

TypeManager::~TypeManager()
{
	// TODO Auto-generated destructor stub
}

bool TypeManager::FileShouldBeScanned(const std::string& name) const
{
	// Find the name's extension.
	const char period[] = ".";
	auto last_period = std::find_end(name.begin(), name.end(),
			period, period+1);
	if(last_period != name.end())
	{
		// There was a period, might be an extension.
		if(last_period != name.begin())
		{
			// Name doesn't start with a period, it still could be an extension.
			auto ext = std::string(last_period, name.end());

			if(m_include_extensions.find(ext) != m_include_extensions.end())
			{
				// Found the extension in the hash of extensions to include.
				return true;
			}
		}
	}

	// Check if the filename is one of the literal filenames we're supposed to look at.
	if(m_included_literal_filenames.find(name) != m_included_literal_filenames.end())
	{
		return true;
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

bool TypeManager::TypeDel(const std::string& type)
{
	m_active_type_map.erase(type);
	auto num_erased = m_builtin_and_user_type_map.erase(type);

	return num_erased > 0;
}

void TypeManager::CompileTypeTables()
{
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
				m_include_extensions.emplace(j, i.first);
			}
			else if(j[0] == '/')
			{
				// First char is a '/', it's a first-line regex.
				m_included_first_line_regexes.emplace(j, i.first);
			}
			else
			{
				// It's a literal filename (e.g. "Makefile").
				m_included_literal_filenames.emplace(j, i.first);
			}
		}
	}
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
