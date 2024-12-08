/*
 * Copyright 2015-2022 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file ArgParse.cpp
 * This is the implementation of the ArgParse class.
 */

#include <config.h>

#include "ArgParse.h"

#include "../build_info.h"

#include "libext/cpuidex.hpp"

// Std C++.
#include <locale>
#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <locale>
#include <thread>
#include <iostream>
#include <sstream>
#include <system_error>
#include <cstdlib>
#include <cstring>
#include <cstdio>


// Include "The Lean Mean C++ Option Parser".
// The namespaces here were originally to avoid clashes with "struct option" in argp.h during the transition away from it,
// but "option" is so common a name that we'll just leave this in place even though we've removed argp.
namespace lmcppop_int {
#include <optionparser.h>
}
namespace lmcppop = lmcppop_int::option;

#if HAVE_LIBPCRE == 1
#include <pcre.h>
#endif
#if HAVE_LIBPCRE2 == 1
#include "FileScannerPCRE2.h"
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include <libext/string.hpp>
#include <libext/filesystem.hpp>

#include "TypeManager.h"
#include "File.h"
#include <libext/Logger.h>
#include <libext/Terminal.h>


// The sweet spot for the number of directory tree traversal threads seems to be 4 on Linux with the new DirTree implementation.
static constexpr size_t f_default_dirjobs = 4;


/// @TODO De-literalize the copyright dates.
static constexpr char f_program_version[] = PACKAGE_STRING "\n"
	"Copyright (C) 2015-2022 Gary R. Van Sickle.\n"
	"\n"
	"This program is free software; you can redistribute it and/or modify\n"
	"it under the terms of version 3 of the GNU General Public License as\n"
	"published by the Free Software Foundation.\n"
	"\n"
	"This program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License\n"
	"along with this program. If not, see http://www.gnu.org/licenses/."
	;

static constexpr char f_program_bug_address[] = PACKAGE_BUGREPORT;

/**
 * The pre- and post-option help text.
 */
static constexpr char f_doc[] = "\nucg: the UniversalCodeGrep code search tool."
		"\vExit status is 0 if any matches were found, 1 if no matches, 2 or greater on error.";

/**
 * The "Usage:" text.
 */
static constexpr char f_args_doc[] = "PATTERN [FILES OR DIRECTORIES]";

/// Keys for options without short-options.
enum OPT
{
	OPT_RESERVED = 0,
	OPT_UNKNOWN = 0,
	OPT_SECTION = 255,
	OPT_IGNORE_CASE = 1,
	OPT_SMART_CASE,
	OPT_NO_SMART_CASE,
	OPT_HANDLE_CASE,
	OPT_LITERAL,
	OPT_WORDREGEX,
	OPT_COLOR,
	OPT_NOCOLOR,
	OPT_NULLSEP,
	OPT_IGNORE_DIR,
	OPT_NOIGNORE_DIR,
	OPT_IGNORE_FILE,
	OPT_INCLUDE,
	OPT_EXCLUDE,
	OPT_FOLLOW,
	OPT_NOFOLLOW,
	OPT_RECURSE_SUBDIRS,
	OPT_ONLY_KNOWN_TYPES,
	OPT_TYPE,
	OPT_NOENV,
	OPT_TYPE_SET,
	OPT_TYPE_ADD,
	OPT_TYPE_DEL,
	OPT_PERF_DIRJOBS,
	OPT_PERF_SCANJOBS,
	OPT_HELP,
	OPT_HELP_TYPES,
	OPT_USAGE,
	OPT_VERSION,
	OPT_COLUMN,
	OPT_NOCOLUMN,
	OPT_TEST_LOG_ALL,
	OPT_TEST_NOENV_USER,
	OPT_TEST_USE_MMAP,
	OPT_BRACKET_NO_STANDIN
};

/// Status code to use for a bad parameter which terminates the program via argp_failure().
/// Ack returns 255 in this case, so we'll use that instead of BSD's EX_USAGE, which is 64.
#define STATUS_EX_USAGE 255


/// Arg validity checkers.
struct Arg: public lmcppop::Arg
{
	static void printError(const char* msg1, const lmcppop::Option& opt, const char* msg2)
	{
		fprintf(stderr, "%s", msg1);
		fwrite(opt.name, opt.namelen, 1, stderr);
		fprintf(stderr, "%s", msg2);
	}

	static constexpr lmcppop::ArgStatus Unknown(const lmcppop::Option& option, bool msg)
	{
		if (msg) printError("ucg: unrecognized option '", option, "'\nTry `ucg --help\' or `ucg --usage\' for more information.\n");
		return lmcppop::ARG_ILLEGAL;
	}

	static constexpr lmcppop::ArgStatus UnknownArgHook(const lmcppop::Option& option, bool msg)
	{
		if(option.name != nullptr)
		{
			/// @todo std::cout << "Dynamic option: '" << option.name << "'\n";
			return lmcppop::ARG_NONE;
		}
		if(msg) printError("Unknown option '", option, "'\n");
		return lmcppop::ARG_ILLEGAL;
	}

	static constexpr lmcppop::ArgStatus Required(const lmcppop::Option& option, bool msg)
	{
		if (option.arg != nullptr)
			return lmcppop::ARG_OK;

		if (msg) printError("Option '", option, "' requires an argument\n");
		return lmcppop::ARG_ILLEGAL;
	}

	static constexpr lmcppop::ArgStatus NonEmpty(const lmcppop::Option& option, bool msg)
	{
		if (option.arg != nullptr && option.arg[0] != 0)
		  return lmcppop::ARG_OK;

		if (msg) printError("Option '", option, "' requires a non-empty argument\n");
		return lmcppop::ARG_ILLEGAL;
	}

	static constexpr lmcppop::ArgStatus Numeric(const lmcppop::Option& option, bool msg)
	{
		char* endptr = nullptr;
		if (option.arg != nullptr && strtol(option.arg, &endptr, 10)){};
		if (endptr != option.arg && *endptr == 0)
		  return lmcppop::ARG_OK;

		if (msg) printError("Option '", option, "' requires a numeric argument\n");
		return lmcppop::ARG_ILLEGAL;
	}

	template <long limit>
	static constexpr lmcppop::ArgStatus IntegerGreater(const lmcppop::Option& option, bool msg)
	{
		if (option.arg != nullptr)
		{
			try
			{
				long val = std::stol(option.arg);
				if(val > limit)
				{
					return lmcppop::ARG_OK;
				}
				else
				{
					throw std::runtime_error("");
				}
			}
			catch(...) { } // Don't do anything in the catch.  If we haven't returned above, we error out below.
		}

		if (msg)
		{
			std::cerr << "Option '" << std::string(option.name, option.namelen) << "' requires an integer argument greater than " << limit << "\n";
		}
		return lmcppop::ARG_ILLEGAL;
	}
};

enum OptionType { UNSPECIFIED = 0, DISABLE = 0, ENABLE = 1,
					IGNORE = 1, SMART_CASE = 2, NO_SMART_CASE = 3 };

static constexpr char m_opt_start_str[] {"  \t"};
static constexpr char m_help_space_str[] {"      \t"};

static std::vector<std::shared_ptr<std::string>> f_delete_us;

struct PreDescriptor
{
	const unsigned m_index;
	const int m_type;
	const int m_notype {0};
	const char* const m_shortopts;
	const char* const m_longopts;
	const char* const m_argname;
	const lmcppop::CheckArg m_check_arg {};
	/// @note The use of std::string here is at least one problem with making the whole option parser description
	/// constexpr.  As of up to C++23, allocations can't escape the constexpr, so this can't be initialized in the
	/// constructors.  I *think* std::string simply can't work here for constexpr, regardless of any machinations we
	/// might try.
	const std::string m_help;
	/// IGNORE ME - constexpr experimentation.
	//	const char* m_help;
	const bool m_is_hidden { false };
	const bool m_is_bracket_no { false };

	struct section_header_tag {};
	struct normal_option_tag {};
	struct arbtext_tag {};
	struct hidden_tag {};

//// IGNORE ME - constexpr experimentation.
//	constexpr const char* help_helper(const char* a) { return a; }; //const auto* retval = new std::string(a); return retval->c_str(); };

//	constexpr explicit PreDescriptor() noexcept
//		: m_index(1), m_type(1), m_shortopts(""), m_longopts(""), m_argname(""), m_check_arg(NULL), m_help(help_helper(""))
//	{
//
//	}

	constexpr PreDescriptor(unsigned index, int type, const char *const shortopts, const char *const longopts,
			const lmcppop::CheckArg check_arg, const char *help) noexcept
			: m_index(index), m_type(type), m_shortopts(shortopts), m_longopts(longopts), m_argname(""), m_check_arg(
			check_arg), m_help(help)
	{
	};

	/**
	 * The generic option constructor.
	 *
	 * @param index
	 * @param type
	 * @param shortopts
	 * @param longopts
	 * @param argname
	 * @param check_arg
	 * @param help
	 */
	constexpr PreDescriptor(unsigned index, int type, const char *const shortopts, const char *const longopts, const char *const argname,
			const lmcppop::CheckArg check_arg, const char *help) noexcept
			: m_index(index), m_type(type), m_shortopts(shortopts), m_longopts(longopts), m_argname(argname), m_check_arg(check_arg), m_help(help)
	{
	};

	/**
	 * The "bracketed-no" constructor.  This is for options which look like this: "--[no]option=ARG".
	 *
	 * @param index
	 * @param type
	 * @param shortopts
	 * @param longopts
	 * @param argname
	 * @param check_arg
	 * @param help
	 * @param
	 */
	constexpr PreDescriptor(unsigned index, OptionType yestype, OptionType notype, const char *const shortopts, const char *const longopts, const char *const argname,
			const lmcppop::CheckArg check_arg, const char *help) noexcept
			: m_index(index), m_type(yestype), m_notype(notype), m_shortopts(shortopts), m_longopts(longopts), m_argname(argname),
			  m_check_arg(check_arg), m_help(help), m_is_bracket_no(true)
	{
	};

	/**
	 * The generic hidden option constructor.
	 *
	 * @param index
	 * @param type
	 * @param shortopts
	 * @param longopts
	 * @param argname
	 * @param check_arg
	 * @param help
	 * @param
	 */
	constexpr PreDescriptor(unsigned index, int type, const char *const shortopts, const char *const longopts, const char *const argname,
			const lmcppop::CheckArg check_arg, const char *help, hidden_tag) noexcept
			: m_index(index), m_type(type), m_shortopts(shortopts), m_longopts(longopts), m_argname(argname), m_check_arg(check_arg), m_help(help),
			  m_is_hidden(true)
	{
	};

	/**
	 * Constructor overload for section headers.
	 *
	 * @note This cannot be marked explicit because of the way we want to use it.
	 *
	 * @param section_header_name  Text of the section header.
	 */
	constexpr PreDescriptor(const char* section_header_name, section_header_tag = section_header_tag()) noexcept
		: m_index(255), m_type(0), m_shortopts(""), m_longopts(""), m_argname(""), m_check_arg(Arg::None),
		  m_help(std::string("\n ") += section_header_name)
	{
	};

	constexpr PreDescriptor(const char *arbitrary_text, arbtext_tag) noexcept
		: m_index(255), m_type(0), m_shortopts(""), m_longopts(""), m_argname(""), m_check_arg(Arg::None),
		  m_help(arbitrary_text)
	{
	};

	constexpr PreDescriptor(const char *arbitrary_text [[maybe_unused]], hidden_tag) noexcept
			: m_index(255), m_type(0), m_shortopts(""), m_longopts(""), m_argname(""), m_check_arg(Arg::None),
			  m_help(), m_is_hidden(true)
	{
	};

	[[nodiscard]] constexpr bool IsHidden() const noexcept { return m_is_hidden; };
	[[nodiscard]] constexpr bool IsBracketNo() const noexcept { return m_is_bracket_no; };
	[[nodiscard]] constexpr bool HasLongAliases() const noexcept
	{
		std::string_view temp_sv(m_longopts);
		return temp_sv.rfind(',') != std::string_view::npos;
	};

	/**
	 * Conversion operator for converting between PreDescriptors and "The Lean Mean C++ Option Parser"'s option Descriptors.
	 */
	operator lmcppop::Descriptor() const noexcept
	{
		const char *fmt_help = "";

		if(IsHidden())
		{
			fmt_help = nullptr;
		}
		else
		{
			auto short_len = std::strlen(m_shortopts);
			auto long_len = std::strlen(m_longopts);

			if((short_len!=0 || long_len!=0) && (!m_help.empty()))
			{
				// This is a non-hidden option.
				// Paste together the options and the help text.
				std::string opts_help_str {m_opt_start_str};
				if(short_len)
				{
					opts_help_str += "-";
					opts_help_str += join(split(m_shortopts, ','), ", -");
				}
				if(short_len && long_len)
				{
					opts_help_str += ", ";
				}
				if(long_len)
				{
					auto longopt_list = split(m_longopts, ',');
					// Check if we need to append an arg expression.
					if(std::strlen(m_argname) > 0)
					{
						for(auto& opt : longopt_list)
						{
							opt += "=";
							opt += m_argname;
						}
					}
					opts_help_str += "--";
					opts_help_str += join(longopt_list, ", --");
				}
				std::shared_ptr<std::string> semi_fmt_help = std::make_shared<std::string>(
						opts_help_str + m_help_space_str + m_help);
				fmt_help = semi_fmt_help->c_str();

				/// @todo We should control this lifetime in a lighter-weight way.
				f_delete_us.push_back(semi_fmt_help);
			}
			else if(!m_help.empty())
			{
				// It's a section header.
				auto semi_fmt_help = std::make_shared<std::string>(m_help);
				fmt_help = semi_fmt_help->c_str();
				f_delete_us.push_back(semi_fmt_help);
			}
			else if(m_help.empty())
			{
				auto semi_fmt_help = std::make_shared<std::string>("");
				fmt_help = semi_fmt_help->c_str();
				f_delete_us.push_back(semi_fmt_help);
			}
		}

		// We may have to change the long option string we pass to LMC++OP.  It only handles
		// a single long option, while our layer here adds the capabilities of option aliases and --[no]opt style options.
		std::shared_ptr<std::string> desc_longopt;

		if(HasLongAliases())
		{
			// Only register the first long option here.
			desc_longopt = std::make_shared<std::string>(m_longopts, std::strchr(m_longopts, ',')-m_longopts);
			if(IsBracketNo())
			{
				// Strip out the '[no]', this will be the 'yes' option.
				desc_longopt->erase(0,4);
			}
			f_delete_us.push_back(desc_longopt);
		}

		size_t bracket_no_offset {0};
		if(IsBracketNo())
		{
			// Make this the "yes" case.  The hidden ones will be --noopt and --no-opt.
			bracket_no_offset = 4;
		}

		return lmcppop::Descriptor {m_index, m_type, m_shortopts, desc_longopt ? desc_longopt->c_str() : (m_longopts + bracket_no_offset),
				m_check_arg, fmt_help};
	}

	template <typename T>
	void PushAliasDescriptors(T* usage_container) const
	{
		auto long_aliases = split(m_longopts, ',');

		for(auto it = long_aliases.begin()+1; it != long_aliases.end(); ++it)
		{
			auto long_alias = std::make_shared<std::string>(*it);
			if(IsBracketNo())
			{
				// For bracket-no aliases, we have to add the --opt, --no-opt, and --noopt hidden entries.
				long_alias->erase(0, 4);
				auto no_opt_str = std::make_shared<std::string>("no-" + *long_alias);
				auto noopt_str = std::make_shared<std::string>("no" + *long_alias);
				f_delete_us.push_back(no_opt_str);
				f_delete_us.push_back(noopt_str);
				usage_container->push_back(lmcppop::Descriptor{m_index, m_notype, "", no_opt_str->c_str(), m_check_arg, 0});
				usage_container->push_back(lmcppop::Descriptor{m_index, m_notype, "", noopt_str->c_str(), m_check_arg, 0});
			}
			f_delete_us.push_back(long_alias);
			usage_container->push_back(lmcppop::Descriptor{m_index, m_type, "", long_alias->c_str(), m_check_arg, 0});
		}
	}

	template <typename T>
	void PushBracketNoDescriptors(T* usage_container) const
	{
		auto long_alias = split(m_longopts, ',')[0];
		long_alias.erase(0, 4);
		auto no_opt_str = std::make_shared<std::string>("no-" + long_alias);
		auto noopt_str = std::make_shared<std::string>("no" + long_alias);
		f_delete_us.push_back(no_opt_str);
		f_delete_us.push_back(noopt_str);
		usage_container->push_back(lmcppop::Descriptor{m_index, m_notype, "", no_opt_str->c_str(), m_check_arg, 0});
		usage_container->push_back(lmcppop::Descriptor{m_index, m_notype, "", noopt_str->c_str(), m_check_arg, 0});
	}

	static constexpr lmcppop::Descriptor NullEntry() noexcept { return lmcppop::Descriptor{0,0,0,0,0,0}; };
};

//// IGNORE ME - constexpr experimentation.
//static constexpr PreDescriptor p;
//static constexpr std::array fcx_raw_options = std::to_array<PreDescriptor>({p});

/// The array of all command line options.
/// @todo It should be possible to make this constexpr, moving a lot of startup work to compile-time.
/// Currently one problem there is that even just a PreDescriptor of {"SomeString:"} isn't const enough.
static const std::array f_raw_options = std::to_array<PreDescriptor>({
	// This first OPT_UNKNOWN entry picks up all unrecognized options.
	{ OPT_UNKNOWN,                                                         0, "", "", "", Arg::Unknown, "", PreDescriptor::hidden_tag() },
	{ (std::string("Usage: ucg [OPTION...] ") += f_args_doc).c_str(),      PreDescriptor::arbtext_tag() },
	// This next one is pretty crazy just to keep the f_doc[] string in the same format as used by argp.
	{ std::string(f_doc).substr(0, std::string(f_doc).find('\v')).c_str(), PreDescriptor::arbtext_tag() },
	{ "Searching:" },
		{ OPT_HANDLE_CASE, SMART_CASE, NO_SMART_CASE, "", "[no]smart-case", "", Arg::None, "Ignore case if PATTERN is all lowercase (default: enabled)." },
		{ OPT_HANDLE_CASE, IGNORE, "i", "ignore-case", Arg::None, "Ignore case distinctions in PATTERN." },
		{ OPT_WORDREGEX, 0, "w", "word-regexp", Arg::None, "PATTERN must match a complete word."},
		{ OPT_LITERAL, 0, "Q", "literal", Arg::None, "Treat all characters in PATTERN as literal."},
	{ "Search Output:" },
		{ OPT_COLUMN, ENABLE, "", "column", Arg::None, "Print column of first match after line number."},
		{ OPT_COLUMN, DISABLE, "", "nocolumn", Arg::None, "Don't print column of first match (default)."},
	{ "File presentation:" },
		{ OPT_COLOR, ENABLE, "", "color,colour", Arg::None, "Render the output with ANSI color codes."},
		{ OPT_COLOR, DISABLE, "", "nocolor,nocolour", Arg::None, "Render the output without ANSI color codes."},
                { OPT_NULLSEP, ENABLE, "", "null", Arg::None,
                  "Print a zero character '\0' instead of a colon ':' after a file name."},
	{ "File/directory inclusion/exclusion:" },
		{ OPT_IGNORE_DIR, ENABLE, DISABLE, "", "[no]ignore-dir,[no]ignore-directory", "NAME", Arg::NonEmpty, "[Do not] exclude directories with NAME."},
		// grep-style --include=glob and --exclude=glob
		{ OPT_INCLUDE, 0, "", "include", "GLOB", Arg::NonEmpty, "Only files matching GLOB will be searched."},
		{ OPT_EXCLUDE, 0, "", "exclude,ignore", "GLOB", Arg::NonEmpty, "Files matching GLOB will be ignored."},  	// ag-style --ignore=GLOB
																													// In ag, this option applies to both files and directories.  For the present, ucg will only apply this to files.
		// ack-style --ignore-file=FILTER:FILTERARGS
		{ OPT_IGNORE_FILE, 0, "", "ignore-file", "FILTER:FILTERARGS", Arg::NonEmpty, "Files matching FILTER:FILTERARGS (e.g. ext:txt,cpp) will be ignored." },
		{ OPT_RECURSE_SUBDIRS, ENABLE, "r,R", "recurse", Arg::None, "Recurse into subdirectories (default: on)." },
		{ OPT_RECURSE_SUBDIRS, DISABLE, "n", "no-recurse", Arg::None, "Do not recurse into subdirectories."},
		{ OPT_FOLLOW, ENABLE, DISABLE, "", "[no]follow", "", Arg::None, "[Do not] follow symlinks (default: nofollow)." },
		{ OPT_ONLY_KNOWN_TYPES, ENABLE, "k", "known-types", Arg::None, "Only search in files of recognized types (default: on)."},
		{ OPT_TYPE, ENABLE, "", "type", "[no]TYPE", Arg::NonEmpty, "Include only [exclude all] TYPE files.  Types may also be specified as --[no]TYPE."},
	{ "File type specification:" },
		{OPT_TYPE_SET, 0, "", "type-set", "TYPE:FILTER:FILTERARGS", Arg::NonEmpty, "Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is replaced."},
		{OPT_TYPE_ADD, 0, "", "type-add", "TYPE:FILTER:FILTERARGS", Arg::NonEmpty, "Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is appended to."},
		{OPT_TYPE_DEL, 0, "", "type-del", "TYPE", Arg::NonEmpty, "Remove any existing definition of type TYPE."},
	{ "Performance tuning:" },
		{ OPT_PERF_DIRJOBS, 0, "", "dirjobs", "NUM_JOBS", Arg::IntegerGreater<0>, "Number of directory traversal jobs (std::thread<>s) to use." },
		{ OPT_PERF_SCANJOBS, 0, "j", "jobs", "NUM_JOBS", Arg::IntegerGreater<0>, "Number of scanner jobs (std::thread<>s) to use."},
	{ "Miscellaneous:" },
		{ OPT_NOENV, 0, "", "noenv", Arg::None, "Ignore .ucgrc configuration files."},
	{ "Informational options:" },
		{ OPT_HELP,  0, "?", "help", Arg::None, "Give this help list" },
		{ OPT_HELP_TYPES, 0, "", "help-types,list-file-types", Arg::None, "Print list of supported file types." },  // --list-file-types for ag compatibility.
		{ OPT_USAGE, 0, "", "usage", Arg::None, "Give a short usage message"},
		{ OPT_VERSION, 0, "V", "version", Arg::None, "Print program version"},
	{ "Hidden Options:", PreDescriptor::hidden_tag() },
	// Hidden options for debug, test, etc.
	// DO NOT USE THESE.  They're going to change and go away without notice.
		{ OPT_TEST_LOG_ALL, 0, "", "test-log-all", "", Arg::None, "Enable all logging output.", PreDescriptor::hidden_tag() },
		{ OPT_TEST_NOENV_USER, 0, "", "test-noenv-user", "", Arg::None, "Don't search for or use $HOME/.ucgrc.", PreDescriptor::hidden_tag() },
		{ OPT_TEST_USE_MMAP, 0, "", "test-use-mmap", "", Arg::None, "Use mmap() to access files being searched.", PreDescriptor::hidden_tag() },
	// Epilogue Text.
		{ "\n" "Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options." "\n", PreDescriptor::arbtext_tag() },
		// Again, this folderol is to keep the f_doc[] string in the same format as used by argp.
		{ (std::string(f_doc).substr(std::string(f_doc).find('\v')+1, std::string::npos) += "\n").c_str(), PreDescriptor::arbtext_tag() },
		{ ((std::string("Report bugs to ") += f_program_bug_address) += ".").c_str(), PreDescriptor::arbtext_tag() }
});

/// Option descriptions for the "The Lean Mean C++ Option Parser" library.
static std::vector<lmcppop::Descriptor> dynamic_usage;


ArgParse::ArgParse(TypeManager &type_manager)
	: m_type_manager(type_manager)
{
	for(auto& ro : f_raw_options)
	{
		/// @note We may want to only push hidden options below, since they break the help formatting into sections.
		/// If we do, we need to make sure the OPT_UNKNOWN handler is the first one.
		lmcppop::Descriptor d = ro;
		dynamic_usage.push_back(d);
	}
	for(auto& ro : f_raw_options)
	{
		if(ro.HasLongAliases())
		{
			ro.PushAliasDescriptors<std::vector<lmcppop::Descriptor>>(&dynamic_usage);
		}
	}
	for(auto& ro : f_raw_options)
	{
		if(ro.IsBracketNo())
		{
			// Bracket-No option, add the hidden --noopt and --no-opt options.
			ro.PushBracketNoDescriptors(&dynamic_usage);
		}
	}
/** see note above
	for(auto& ro : raw_options)
	{
		if(ro.IsHidden())
		{
			lmcppop::Descriptor d = ro;
			dynamic_usage.push_back(d);
		}
	}
*/
	dynamic_usage.push_back(PreDescriptor::NullEntry());
}


/**
 * Why?  There's a legitimate reason, honestly:
 * 1. Our parser library is expecting an argv array of char*'s.
 * 2. For handling the rc files, we're creating vectors of new[]'ed char*'s.
 * 3. main's argv of course is an array of char*'s of unknown allocation.
 * 4. We combine #2 and #3 into one big vector<char*>, pass it to argp_parse().
 * 5. Then when we're all done, we have to delete[] all these strings, and we don't want to have to separately track and free() the ones from
 *    main()'s argv that we strdup()'ed.
 * @param orig  The string to be duplicated.
 * @return  A new[]-allocated copy of #orig.
 */
static char * cpp_strdup(const char *orig)
{
	char *retval = new char[strlen(orig)+1];
	std::strcpy(retval, orig);
	return retval;
}


void ArgParse::Parse(int argc, char **argv)
{
	std::vector<char*> user_argv, project_argv, combined_argv;

	// Check the command line for the --noenv option.
	// Note that we have to handle 'ucg -- --noenv' properly, hence the one_past_end_or_double_dash search first.
	auto one_past_end_or_double_dash = std::find_if(argv, argv+argc, [](char *s){ return std::strcmp(s, "--") == 0; });
	auto noenv = std::count_if(argv, one_past_end_or_double_dash, [](char *s){ return std::strcmp(s, "--noenv") == 0; });

	// Check for some test options which only make sense on the command line.
	auto noenv_user = std::count_if(argv, one_past_end_or_double_dash, [](char *s){ return std::strcmp(s, "--test-noenv-user") == 0; });
	if(noenv_user != 0)
	{
		m_test_noenv_user = true;
	}

	if(noenv == 0)
	{
		// Read all the config files.
		FindAndParseConfigFiles(nullptr, &user_argv, &project_argv);
	}

	// Combine all the argvs into one.
	combined_argv.push_back(cpp_strdup(argv[0]));
	if(noenv == 0)
	{
		combined_argv.insert(combined_argv.end(), user_argv.begin(), user_argv.end());
		combined_argv.insert(combined_argv.end(), project_argv.begin(), project_argv.end());
	}
	for(int i=1; i<argc; ++i)
	{
		combined_argv.push_back(cpp_strdup(argv[i]));
	}
	//combined_argv.push_back(nullptr);

	// We have to handle User Defined Types and --TYPEs ourselves, before finally calling argp_parse().  argp doesn't
	// support dynamically added options, which we need for the --TYPEs that will be created by the User Defined Type
	// mechanism.
	HandleTYPELogic(&combined_argv);

	/// Now parse the args with "The Lean Mean C++ Option Parser".

	// Determine how big the arrays need to be.
	lmcppop::Stats stats(true /* GNU == parse all non-options after options. */,
			dynamic_usage.data(), combined_argv.size()-1, combined_argv.data()+1,
			2 /* Enable unambiguous prefix option parsing, 2 or more chars. */);

	// Allocate the arrays.
	std::vector<lmcppop::Option> options(stats.options_max);
	std::vector<lmcppop::Option> buffer(stats.buffer_max);

	// Finally parse the options.
	lmcppop::Parser parse(true /* GNU == parse all non-options after options. */,
			dynamic_usage.data(), combined_argv.size()-1, combined_argv.data()+1, options.data(), buffer.data(),
			2 /* Enable unambiguous prefix option parsing, 2 or more chars. */);

 	if (parse.error())
 	{
 		// Command line parsing error.  Possibly an unrecognized option, or not enough non-option params.
 		exit(STATUS_EX_USAGE);
    	return;
 	}

	if (options[OPT_HELP] || argc == 0)
	{
		int columns = Terminal::GetColumns();
		lmcppop::printUsage(std::cout, dynamic_usage.data(), columns);
		exit(0);
		return;
	}
	else if(options[OPT_VERSION])
	{
		PrintVersionText(stdout);
		exit(0);
		return;
	}
	else if(options[OPT_HELP_TYPES])
	{
		PrintHelpTypes();
		exit(0);
		return;
	}
	else if(parse.nonOptionsCount() == 0)
	{
		// Need at least the PATTERN.
		/// @todo print short usage
		int columns = Terminal::GetColumns();
		lmcppop::printUsage(fwrite, stdout, dynamic_usage.data(), columns);
		exit(STATUS_EX_USAGE);
	}

	// Grab the pattern.
	if(parse.nonOptionsCount() > 0)
	{
		m_pattern = parse.nonOption(0);
	}

	// Grab any file/dir paths specified on command line.
	for (int i = 1; i < parse.nonOptionsCount(); ++i)
	{
    	m_paths.push_back(parse.nonOption(i));
	}

	// Handle logging verbosity.
	if(options[OPT_TEST_LOG_ALL])
	{
		INFO::Enable(true);
		DEBUG::Enable(true);
	}
	// Handle --test-use-mmap.
	m_use_mmap = (options[OPT_TEST_USE_MMAP].last()->type() == ENABLE);

	// Work out the interaction between ignore-case and smart-case.
	for(lmcppop::Option* opt = options[OPT_HANDLE_CASE]; opt; opt = opt->next())
	{
		switch(opt->type())
		{
		case IGNORE:
			m_ignore_case = true;
			m_smart_case = false;
			break;
		case SMART_CASE:
			m_smart_case = true;
			m_ignore_case = false;
			break;
		case NO_SMART_CASE:
			m_smart_case = false;
		}
	}

	m_word_regexp = options[OPT_WORDREGEX];
	m_pattern_is_literal = options[OPT_LITERAL];
	m_column = (options[OPT_COLUMN].last()->type() == ENABLE);
	if(options[OPT_COLOR]) // If not specified on command line, defaults to both == false.
	{
		m_color = (options[OPT_COLOR].last()->type() == ENABLE);
		m_nocolor = !m_color;
	}

	if(options[OPT_RECURSE_SUBDIRS]) // m_recurse defaults to true, so only assign if option was really given.
	{
		m_recurse = (options[OPT_RECURSE_SUBDIRS].last()->type() == ENABLE);
	}
	m_follow_symlinks = (options[OPT_FOLLOW].last()->type() == ENABLE);

	for(lmcppop::Option* opt = options[OPT_IGNORE_DIR]; opt; opt=opt->next())
	{
		if(opt->type() == ENABLE)
		{
			m_excludes.insert(opt->arg);
		}
		else
		{
			/**
			 * @todo Ack is fancier in its noignore handling.  If you noignore a directory under an ignored
			 * directory, it gets put back into the set of paths that will be searched.  Feature for another day.
			 */
			m_excludes.erase(opt->arg);
		}
	}

	// No-op currently, default is -k.
	// m_only_known_types = (options[OPT_ONLY_KNOWN_TYPES] == ENABLE);

	for(lmcppop::Option* opt = options[OPT_TYPE]; opt; opt=opt->next())
	{
		if(std::strncmp("no", opt->arg, 2) == 0)
		{
			// The first two chars are "no", this is a "--type=noTYPE" option.
			if(m_type_manager.notype(opt->arg+2) == false)
			{
				std::cerr << "ucg: Unknown type '" << opt->arg+2 << "'.\n";
				exit(STATUS_EX_USAGE);
			}
		}
		else
		{
			// This is a "--type=TYPE" option.
			if(m_type_manager.type(opt->arg) == false)
			{
				std::cerr << "ucg: Unknown type '" << opt->arg << "'.\n";
				exit(STATUS_EX_USAGE);
			}
		}
	}

	if(lmcppop::Option* opt = options[OPT_PERF_DIRJOBS])
	{
		m_dirjobs = std::stoi(opt->arg);
	}
	if(lmcppop::Option* opt = options[OPT_PERF_SCANJOBS])
	{
		m_jobs = std::stoi(opt->arg);
	}

	//// Now set up some defaults which we can only determine after all arg parsing is complete.

	// Number of jobs.
	if(m_jobs == 0)
	{
		// Number of jobs wasn't specified on command line.  Default to the number of cores the std lib says we have.
		m_jobs = std::thread::hardware_concurrency();

		if(m_jobs == 0)
		{
			// std::thread::hardware_concurrency() is broken.  Default to one thread.
			m_jobs = 1;
		}
	}

	// Number of directory scanning jobs.
	if(m_dirjobs == 0)
	{
		// Wasn't specified on command line.  Use the default.
		m_dirjobs = f_default_dirjobs;
	}

	// Search files/directories.
	if(m_paths.empty())
	{
		// Default to current directory.
		m_paths.push_back(".");
	}

	// Is smart-case enabled, and will we otherwise not be ignoring case?
	if(m_smart_case && !m_ignore_case)
	{
		// Is PATTERN all lower-case?

		// Use a copy of the current global locale.  Since we never call std::locale::global() to set it,
		// this should end up defaulting to the "classic" (i.e. "C") locale.
		/// @todo This really should be the environment's default locale (loc("")).  Cygwin doesn't support this
		/// at the moment (loc("") throws), and the rest of ucg isn't localized anyway, so this should work for now.
		std::locale loc;
		// Look for the first uppercase char in PATTERN.
		if(std::find_if(m_pattern.cbegin(), m_pattern.cend(), [&loc](decltype(m_pattern)::value_type c){ return std::isupper(c, loc); }) == m_pattern.cend())
		{
			// Didn't find one, so match without regard to case.
			m_ignore_case = true;
		}
	}

	// Free the strings we cpp_strdup()'ed
	for(char *c : combined_argv)
	{
		delete [] c;
	}
}

void ArgParse::PrintVersionText(FILE* stream)
{
	// Print the version string and copyright notice.
	std::fputs(f_program_version, stream);

	// In addition, we want to print the compiler/version we were built with, the libpcre version and some other info on it,
	// and any source control version info we can get.

	std::fprintf(stream, "\n\nBuild info\n");

	//
	// Provenance info.
	//
	std::fprintf(stream, "\nRepo version: %s\n", g_git_describe);

	//
	// Compiler info
	//
	std::fprintf(stream, "\nCompiler info:\n");
	std::fprintf(stream, " Name ($(CXX)): \"%s\"\n", g_cxx);
	std::fprintf(stream, " Version string: \"%s\"\n", g_cxx_version_str);

	//
	// Runtime info
	//
	std::fprintf(stream, "\nISA extensions in use:\n");
	std::fprintf(stream, " sse4.2: %s\n", sys_has_sse4_2() ? "yes" : "no");
	std::fprintf(stream, " popcnt: %s\n", sys_has_popcnt() ? "yes" : "no");

	//
	// libpcre info
	//
	{
		std::fprintf(stream, "\nlibpcre info:\n");
#if HAVE_LIBPCRE == 0
		std::fprintf(stream, " Not linked against libpcre.\n");
#else
		std::fprintf(stream, " Version: %s\n", pcre_version());
		std::string s;
		int is_jit;
		s = "no";
		if(pcre_config(PCRE_CONFIG_JIT, &is_jit) == 0)
		{
			s = is_jit ? "yes" : "no";
		}
		std::fprintf(stream, " JIT support built in?: %s\n", s.c_str());
		const char *jittarget = "none";
		if(pcre_config(PCRE_CONFIG_JITTARGET, &jittarget) == 0)
		{
			if(jittarget == NULL)
			{
				jittarget = "none";
			}
		}
		std::fprintf(stream, " JIT target architecture: %s\n", jittarget);
		int nl;
		s = "unknown";
		const std::map<const int, const std::string> newline_desc { {10, "LF"}, {13, "CR"}, {3338, "CRLF"}, {-2, "ANYCRLF"}, {-1, "ANY"},
												{21, "LF(EBCDIC)"}, {37, "LF(37)(EBCDIC)"}, {3349, "CRLF(EBCDIC)"}, {3365, "CRLF(37)(EBCDIC)"}};
		if(pcre_config(PCRE_CONFIG_NEWLINE, &nl) == 0)
		{
			auto nl_type_name = newline_desc.find(nl);
			if(nl_type_name != newline_desc.end())
			{
				s = nl_type_name->second;
			}
		}
		std::fprintf(stream, " Newline style: %s\n", s.c_str());
#endif
	}

	//
	// libpcre2 info
	//
	{
		std::fprintf(stream, "\nlibpcre2-8 info:\n");
#if HAVE_LIBPCRE2 == 0
		std::fprintf(stream, " Not linked against libpcre2-8.\n");
#else
		std::fprintf(stream, " Version: %s\n", FileScannerPCRE2::GetPCRE2Version().c_str());

		std::string s;
		uint32_t is_jit;
		s = "no";
		if(pcre2_config(PCRE2_CONFIG_JIT, &is_jit) == 0)
		{
			s = is_jit ? "yes" : "no";
		}
		std::fprintf(stream, " JIT support built in?: %s\n", s.c_str());
		char jittarget[48];
		if(pcre2_config(PCRE2_CONFIG_JITTARGET, jittarget) == PCRE2_ERROR_BADOPTION)
		{
			std::strcpy(jittarget, "none");
		}
		std::fprintf(stream, " JIT target architecture: %s\n", jittarget);
		uint32_t nl;
		s = "unknown";
		const std::map<const uint32_t, const std::string> newline_desc {
			{PCRE2_NEWLINE_LF, "LF"},
			{PCRE2_NEWLINE_CR, "CR"},
			{PCRE2_NEWLINE_CRLF, "CRLF"},
			{PCRE2_NEWLINE_ANYCRLF, "ANYCRLF"},
			{PCRE2_NEWLINE_ANY, "ANY"}
		};
		if(pcre2_config(PCRE2_CONFIG_NEWLINE, &nl) == 0)
		{
			auto nl_type_name = newline_desc.find(nl);
			if(nl_type_name != newline_desc.end())
			{
				s = nl_type_name->second;
			}
		}
		std::fprintf(stream, " Newline style: %s\n", s.c_str());
#endif
	}
}

void ArgParse::PrintHelpTypes() const
{
	std::cout << "ucg recognizes the following file types:" << std::endl;
	std::cout << std::endl;
	m_type_manager.PrintTypesForHelp(std::cout);
	std::cout << std::endl;
}

void ArgParse::FindAndParseConfigFiles(std::vector<char*> */*global_argv*/, std::vector<char*> *user_argv, std::vector<char*> *project_argv) const
{
	// Find and parse the global config file.
	/// @todo
	/// @note global_argv commented out above to avoid unused parameter warning.

	// Check if we're ignoring $HOME/.ucgrc for test purposes.
	if(!m_test_noenv_user)
	{
		// Parse the user's config file.
		std::string homedir = portable::get_home_dir_name();
		if(!homedir.empty())
		{
			// See if we can open the user's .ucgrc file.
			std::string homefilepath = homedir + "/.ucgrc";
			try
			{
				File home_file(homefilepath, FAM_RDONLY, FCF_NOATIME | FCF_NOCTTY);

				if(home_file.size() == 0)
				{
					LOG(INFO) << "Config file \"" << homefilepath << "\" is zero-length.";
				}
				else
				{
					auto vec_argv = ConvertRCFileToArgv(home_file);

					user_argv->insert(user_argv->end(), vec_argv.cbegin(), vec_argv.cend());
				}
			}
			catch(const FileException &e)
			{
				if(e.code() != std::errc::no_such_file_or_directory)
				{
					WARN() << "Couldn't open config file \"" << homefilepath << "\", error " << e.code() << " - " << e.code().message();
				}
				else
				{
					// .ucgrc file doesn't exist.
					LOG(INFO) << "During search for ~/.ucgrc: " << e;
				}
			}
			catch(const std::system_error &e)
			{
				/// @todo Do we need this anymore?  It should be dealt with above in the FileException catch.
				if(e.code() != std::errc::no_such_file_or_directory)
				{
					WARN() << "Couldn't open config file \"" << homefilepath << "\", error " << e.code() << " - " << e.code().message();
				}
				// Otherwise, the file just doesn't exist.
				else
				{
					LOG(DEBUG) << "file doesn't exist";
				}
			}
		}
	}

	// Find and parse the project config file.
	auto proj_rc_filename = GetProjectRCFilename();
	if(!proj_rc_filename.empty())
	{
		// We found it, see if we can open it.
		try
		{
			File proj_rc_file(proj_rc_filename, FAM_RDONLY, FCF_NOATIME | FCF_NOCTTY);

			if(proj_rc_file.size() == 0)
			{
				LOG(INFO) << "Config file \"" << proj_rc_filename << "\" is zero-length.";
			}
			else
			{
				LOG(INFO) << "Parsing config file \"" << proj_rc_filename << "\".";

				auto vec_argv = ConvertRCFileToArgv(proj_rc_file);

				project_argv->insert(project_argv->end(), vec_argv.cbegin(), vec_argv.cend());
			}
		}
		catch(const FileException &e)
		{
			WARN() << "During search for project .ucgrc file: " << e.what();
		}
		catch(const std::system_error &e)
		{
			if(e.code() != std::errc::no_such_file_or_directory)
			{
				WARN() << "Couldn't open config file \"" << proj_rc_filename << "\", error " << e.code() << " - " << e.code().message();
			}
			// Otherwise, the file just doesn't exist.
		}
	}
}


std::string ArgParse::GetProjectRCFilename()
{
	// Walk up the directory hierarchy from the cwd until we:
	// 1. Get to the user's $HOME dir, in which case we don't return an rc filename even if it exists.
	// 2. Find an rc file, which we'll then return the name of.
	// 3. Can't go up the hierarchy any more (i.e. we hit root).
	/// @todo We might want to reconsider if we want to start at cwd or rather at whatever
	///       paths may have been specified on the command line.  cwd is what Ack is documented
	///       to do, and is easier.

	std::string retval;

	// Get a file descriptor to the user's home dir, if there is one.
	std::string homedirname = portable::get_home_dir_name();
	int home_fd = -1;
	if(!homedirname.empty())
	{
		home_fd = open(homedirname.c_str(), O_RDONLY);
		/// @todo Should probably check for is-a-dir here.
	}

	// Get the current working directory's absolute pathname.
	/// @note GRVS - get_current_dir_name() under Cygwin will currently return a DOS path if this is started
	///              under the Eclipse gdb.  This mostly doesn't cause problems, except for terminating the loop
	///              (see below).
	std::string current_cwd = portable::get_current_dir_name();

	while(!current_cwd.empty() && current_cwd[0] != '.')
	{
		// If we were able to get a file descriptor to $HOME above...
		if(home_fd != -1)
		{
			// ...check if this dir is the user's $HOME dir.
			int cwd_fd = open(current_cwd.c_str(), O_RDONLY);
			if(cwd_fd != -1)
			{
				/// @todo Should probably check for is-a-dir here.
				if(is_same_file(cwd_fd, home_fd))
				{
					// We've hit the user's home directory without finding a config file.
					close(cwd_fd);
					break;
				}
				close(cwd_fd);
			}
			// else couldn't open the current cwd, so we can't check if it's the same directory as home_fd.
		}

		// Try to open the config file.
		auto test_rc_filename = current_cwd;
		if(*test_rc_filename.rbegin() != '/')
		{
			test_rc_filename += "/";
		}
		test_rc_filename += ".ucgrc";
		LOG(INFO) << "Checking for rc file \'" << test_rc_filename << "\'";
		int rc_file = open(test_rc_filename.c_str(), O_RDONLY);
		if(rc_file != -1)
		{
			// Found it.  Return its name.
			LOG(INFO) << "Found rc file \'" << test_rc_filename << "\'";
			retval = test_rc_filename;
			close(rc_file);
			break;
		}

		/// @note GRVS - get_current_dir_name() under Cygwin will currently return a DOS path if this is started
		///              under the Eclipse gdb.  This mostly doesn't cause problems, except for terminating the loop.
		///              The clause below after the || handles this.
		if((current_cwd.length() == 1) || (current_cwd.length() <= 4 && current_cwd[1] == ':'))
		{
			// We've hit the root and didn't find a config file.
			break;
		}

		// Go up one directory.
		current_cwd = portable::dirname(current_cwd);
	}

	if(home_fd != -1)
	{
		// Close the homedir we opened above.
		close(home_fd);
	}

	return retval;
}

std::vector<char*> ArgParse::ConvertRCFileToArgv(const File& f)
{
	// The RC files we support are text files with one command-line parameter per line.
	// Comments are supported, as lines beginning with '#'.
	const char *pos = f.data();
	const char *end = f.data() + f.size();
	std::vector<char*> retval;

	while(pos != end)
	{
		// Eat all leading whitespace, including newlines.
		pos = std::find_if_not(pos, end, [](char c){ return std::isspace(c); });

		if(pos != end)
		{
			// We found some non-whitespace text.
			if(*pos == '#')
			{
				// It's a comment, skip it.
				pos = std::find_if(pos, end, [](char c){ return c == '\n'; });
				// pos now points at the '\n', but the eat-ws line above will consume it.
			}
			else
			{
				// It's something that is intended to be a command-line param.
				// Find the end of the line.  Everything between [pos, param_end) is the parameter.
				const char *param_end = std::find_if(pos, end, [](char c){ return (c=='\r') || (c=='\n'); });

				char *param = new char[(param_end - pos) + 1];
				std::copy(pos, param_end, param);
				param[param_end - pos] = '\0';

				// Check if it's not an option.
				if(std::strcmp(param, "--") == 0)
				{
					// Double-dash is not allowed in an rc file.
					throw ArgParseException("Double-dash \"" + std::string(param) + "\" is not allowed in rc file \"" + f.name() + "\".");
				}
				else if(param[0] != '-')
				{
					throw ArgParseException("Non-option argument \"" + std::string(param) + "\" is not allowed in rc file \"" + f.name() + "\".");
				}

				retval.push_back(param);

				pos = param_end;
			}
		}
	}

	return retval;
}

void ArgParse::HandleTYPELogic(std::vector<char*> *v)
{
	for(auto arg = v->begin(); arg != v->end(); ++arg)
	{
		try // TypeManager might throw on malformed file type filter strings.
		{
			auto arglen = std::strlen(*arg);
			if((arglen < 3) || (std::strncmp("--", *arg, 2) != 0))
			{
				// We only care about double-dash options here.
				continue;
			}

			if(std::strcmp("--", *arg) == 0)
			{
				// This is a "--", ignore all further command-line params.
				break;
			}

			std::string argtxt(*arg+2);

			// Is this a type specification of the form "--TYPE"?
			auto type_name_list = m_type_manager.GetMatchingTypenameList(argtxt);
			if(type_name_list.size() == 1)
			{
				// Yes, replace it with something digestible by argp: --type=TYPE.
				std::string new_param("--type=" + type_name_list[0]);
				delete [] *arg;
				*arg = cpp_strdup(new_param.c_str());
			}
			else if(type_name_list.size() > 1)
			{
				// Ambiguous parameter.
				// Try to match argp's output in this case as closely as we can.
				// argp's output in such a case looks like this:
				//   $ ./ucg --i 'endif' ../
				//   ./ucg: option '--i' is ambiguous; possibilities: '--ignore-case' '--ignore' '--include' '--ignore-file' '--ignore-directory' '--ignore-dir'
				//   Try `ucg --help' or `ucg --usage' for more information.
				std::string possibilities = "'--" + join(type_name_list, "' '--") + "'";
				throw ArgParseException((std::string("option '--") += argtxt += "' is ambiguous; possibilities: ") += possibilities);
			}

			// Is this a type specification of the form '--noTYPE'?
			else if(argtxt.compare(0, 2, "no") == 0)
			{
				auto possible_type_name = argtxt.substr(2);
				auto type_name_list = m_type_manager.GetMatchingTypenameList(possible_type_name);
				if(type_name_list.size() == 1)
				{
					// Yes, replace it with something digestible by argp: --type=noTYPE.
					std::string new_param("--type=no" + type_name_list[0]);
					delete [] *arg;
					*arg = cpp_strdup(new_param.c_str());
				}
				else if(type_name_list.size() > 1)
				{
					// Ambiguous parameter.
					std::string possibilities = "'--no" + join(type_name_list, "' '--no") + "'";
					throw ArgParseException("option '--" + argtxt + "' is ambiguous; possibilities: " + possibilities);
				}
			}

			// Otherwise, check if it's one of the file type definition parameters.
			else
			{
				// All file type params take the form "--CMD=CMDPARAMS", so split on the '='.
				auto on_equals_split = split(argtxt, '=');

				if(on_equals_split.size() != 2)
				{
					// No '=', not something we care about here.
					continue;
				}

				// Is this a type-add?
				if(on_equals_split[0] == "type-add")
				{
					// Yes, have type manager add the params.
					m_type_manager.TypeAddFromFilterSpecString(false, on_equals_split[1]);
				}

				// Is this a type-set?
				else if(on_equals_split[0] == "type-set")
				{
					// Yes, delete any existing file type by the given name and do a type-add.
					m_type_manager.TypeAddFromFilterSpecString(true, on_equals_split[1]);
				}

				// Is this a type-del?
				else if(on_equals_split[0] == "type-del")
				{
					// Tell the TypeManager to delete the type.
					/// @note ack reports no error if the file type to be deleted doesn't exist.
					/// We'll match that behavior here.
					m_type_manager.TypeDel(on_equals_split[1]);
				}

				// Is this an ignore-file?
				else if(on_equals_split[0] == "ignore-file")
				{
					// It's an ack-style "--ignore-file=FILTER:FILTERARGS".
					// Behaviorally, this is as if an unnamed type was set on the command line, and then
					// immediately --notype='ed.  So that's how we'll handle it.
					m_type_manager.TypeAddIgnoreFileFromFilterSpecString(on_equals_split[1]);
				}

				// Is this an include or exclude?
				else if(on_equals_split[0] == "exclude" || on_equals_split[0] == "ignore")
				{
					// This is a grep-style "--exclude=GLOB" or an ag-style "--ignore=GLOB".
					m_type_manager.TypeAddIgnoreFileFromFilterSpecString("globx:" + on_equals_split[1]);
				}
				else if(on_equals_split[0] == "include")
				{
					// This is a grep-style "--include=GLOB".
					m_type_manager.TypeAddIncludeGlobFromFilterSpecString("glob:" + on_equals_split[1]);
				}
			}
		}
		catch(const TypeManagerException &e)
		{
			throw ArgParseException(std::string(e.what()) + " while parsing option \'" + *arg + "\'");
		}
	}
}
