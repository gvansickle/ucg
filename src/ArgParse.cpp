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

/** @file ArgParse.cpp
 * This is the implementation of the ArgParse class.  Because of the use of GNU argp (a C library) for arg parsing,
 * there's a healthy mix of C in here as well as C++; a tribute to the interoperability of the two languages.
 */

#include <config.h>

#include "ArgParse.h"

#include "../build_info.h"

#include <algorithm>
#include <vector>
#include <string>
#include <locale>
#include <thread>
#include <iostream>
#include <sstream>
#include <system_error>

#include <argp.h>
#ifdef HAVE_LIBPCRE
#include <pcre.h>
#endif
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef HAVE_PWD_H
#include <pwd.h> // for GetUserHomeDir()-->getpwuid().
#endif

#include <fcntl.h>
#include <unistd.h> // for GetUserHomeDir()-->getuid().
#include <sys/stat.h>
#include <libgen.h>   // Don't know where "libgen" comes from, but this is where POSIX says dirname() and basename() are declared.

#include "TypeManager.h"
#include "File.h"


// Our --version output isn't just a static string, so we'll register with argp for a version callback.
static void PrintVersionTextRedirector(FILE *stream, struct argp_state *state)
{
	static_cast<ArgParse*>(state->input)->PrintVersionText(stream);
}
void (*argp_program_version_hook)(FILE *stream, struct argp_state *state) = PrintVersionTextRedirector;


// Not static, argp.h externs this.
const char *argp_program_version = PACKAGE_STRING "\n"
	"Copyright (C) 2015-2016 Gary R. Van Sickle.\n"
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

// Not static, argp.h externs this.
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "ucg: the UniversalCodeGrep tool.";

static char args_doc[] = "PATTERN [FILES OR DIRECTORIES]";

/// Keys for options without short-options.
enum OPT
{
	OPT_COLOR = 1,
	OPT_NOCOLOR,
	OPT_IGNORE_DIR,
	OPT_NOIGNORE_DIR,
	OPT_TYPE,
	OPT_NOENV,
	OPT_TYPE_SET,
	OPT_TYPE_ADD,
	OPT_TYPE_DEL,
	OPT_HELP_TYPES,
	OPT_COLUMN,
	OPT_NOCOLUMN
};

/// Status code to use for a bad parameter which terminates the program via argp_failure().
/// Ack returns 255 in this case, so we'll use that instead of BSD's EX_USAGE, which is 64.
#define STATUS_EX_USAGE 255

// Not static, argp.h externs this.
error_t argp_err_exit_status = STATUS_EX_USAGE;

/// Argp Option Definitions
// Disable (at least on gcc) the large number of spurious warnings about missing initializers
// the declaration of options[] and ArgParse::argp normally cause.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp_option options[] = {
		{0,0,0,0, "Searching:" },
		{"ignore-case", 'i', 0,	0,	"Ignore case distinctions in PATTERN."},
		{"word-regexp", 'w', 0, 0, "PATTERN must match a complete word."},
		{"literal", 'Q', 0, 0, "Treat all characters in PATTERN as literal."},
		{0,0,0,0, "Search Output:"},
		{"column", OPT_COLUMN, 0, 0, "Print column of first match after line number."},
		{"nocolumn", OPT_NOCOLUMN, 0, 0, "Don't print column of first match (default)."},
		{0,0,0,0, "File presentation:" },
		{"color", OPT_COLOR, 0, 0, "Render the output with ANSI color codes."},
		{"colour", OPT_COLOR, 0, OPTION_ALIAS },
		{"nocolor", OPT_NOCOLOR, 0, 0, "Render the output without ANSI color codes."},
		{"nocolour", OPT_NOCOLOR, 0, OPTION_ALIAS },
		{0,0,0,0, "File inclusion/exclusion:"},
		{"ignore-dir",  OPT_IGNORE_DIR, "name", 0,  "Exclude directories with this name."},
		{"ignore-directory", OPT_IGNORE_DIR, "name", OPTION_ALIAS },
		{"noignore-dir",  OPT_NOIGNORE_DIR, "name", 0,  "Do not exclude directories with this name."},
		{"noignore-directory", OPT_NOIGNORE_DIR, "name", OPTION_ALIAS },
		{"recurse", 'r', 0, 0, "Recurse into subdirectories (default: on)." },
		{0, 'R', 0, OPTION_ALIAS },
		{"no-recurse", 'n', 0, 0, "Do not recurse into subdirectories."},
		{"known-types", 'k', 0, 0, "Only search in files of recognized types (default: on)."},
		{"type", OPT_TYPE, "[no]TYPE", 0, "Include only [exclude all] TYPE files.  Types may also be specified as --[no]TYPE."},
		{0,0,0,0, "File type specification:"},
		{"type-set", OPT_TYPE_SET, "TYPE:FILTER:FILTERARGS", 0, "Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is replaced."},
		{"type-add", OPT_TYPE_ADD, "TYPE:FILTER:FILTERARGS", 0, "Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is appended to."},
		{"type-del", OPT_TYPE_DEL, "TYPE", 0, "Remove any existing definition of type TYPE."},
		{0,0,0,0, "Miscellaneous:" },
		{"noenv", OPT_NOENV, 0, 0, "Ignore .ucgrc files."},
		{"jobs",  'j', "NUM_JOBS",      0,  "Number of scanner jobs (std::thread<>s) to use." },
		{0,0,0,0, "Informational options:", -1}, // -1 is the same group the default --help and --version are in.
		{"help-types", OPT_HELP_TYPES, 0, 0, "Print list of supported file types."},
		{"list-file-types", 0, 0, OPTION_ALIAS }, // For ag compatibility.
		{ 0 }
	};

/// The argp struct for argp.
struct argp ArgParse::argp = { options, ArgParse::parse_opt, args_doc, doc };

#pragma GCC diagnostic pop // Re-enable -Wmissing-field-initializers


error_t ArgParse::parse_opt (int key, char *arg, struct argp_state *state)
{
	class ArgParse *arguments = static_cast<ArgParse*>(state->input);

	switch (key)
	{
	case 'i':
		arguments->m_ignore_case = true;
		break;
	case 'w':
		arguments->m_word_regexp = true;
		break;
	case 'Q':
		arguments->m_pattern_is_literal = true;
		break;
	case OPT_COLUMN:
		arguments->m_column = true;
		break;
	case OPT_NOCOLUMN:
		arguments->m_column = false;
		break;
	case OPT_IGNORE_DIR:
		arguments->m_excludes.insert(arg);
		break;
	case OPT_NOIGNORE_DIR:
		/**
		 * @todo Ack is fancier in its noignore handling.  If you noignore a directory under an ignored
		 * directory, it gets put back into the set of paths that will be searched.  Feature for another day.
		 */
		arguments->m_excludes.erase(arg);
		break;
	case 'r':
	case 'R':
		arguments->m_recurse = true;
		break;
	case 'n':
		arguments->m_recurse = false;
		break;
	case 'k':
		// No argument variable because currently we only support searching known types.
		break;
	case OPT_TYPE:
		if(std::strncmp("no", arg, 2) == 0)
		{
			// The first two chars are "no", this is a "--type=noTYPE" option.
			if(arguments->m_type_manager.notype(arg+2) == false)
			{
				argp_failure(state, STATUS_EX_USAGE, 0, "Unknown type \'%s\'.", arg+2);
			}
		}
		else
		{
			// This is a "--type=TYPE" option.
			if(arguments->m_type_manager.type(arg) == false)
			{
				argp_failure(state, STATUS_EX_USAGE, 0, "Unknown type \'%s\'.", arg);
			}
		}
		break;
	case OPT_TYPE_SET:
	case OPT_TYPE_ADD:
	case OPT_TYPE_DEL:
		// These options are all handled specially outside of the argp parser.
		break;
	case OPT_NOENV:
		// The --noenv option is handled specially outside of the argp parser.
		break;
	case OPT_HELP_TYPES:
		// Consume the rest of the options/args.
		state->next = state->argc;
		arguments->PrintHelpTypes();
		break;
	case 'j':
		if(atoi(arg) < 1)
		{
			// Specified 0 or negative jobs.
			argp_failure(state, STATUS_EX_USAGE, 0, "jobs must be >= 1");
		}
		else
		{
			arguments->m_jobs = atoi(arg);
		}
		break;
	case OPT_COLOR:
		arguments->m_color = true;
		arguments->m_nocolor = false;
		break;
	case OPT_NOCOLOR:
		arguments->m_color = false;
		arguments->m_nocolor = true;
		break;
	case ARGP_KEY_ARG:
		if(state->arg_num == 0)
		{
			// First arg is the pattern.
			arguments->m_pattern = arg;
		}
		else
		{
			// Remainder are optional file paths.
			arguments->m_paths.push_back(arg);
		}
		break;
	case ARGP_KEY_END:
		if(state->arg_num < 1)
		{
			// Not enough args.
			argp_usage(state);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


ArgParse::ArgParse(TypeManager &type_manager)
	: m_type_manager(type_manager)
{
}

ArgParse::~ArgParse()
{
}

/**
 * Why?  There's a legitimate reason, honestly:
 * 1. argp's argp_parse() is expecting an argv array of char*'s.
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
	strcpy(retval, orig);
	return retval;
}

void ArgParse::Parse(int argc, char **argv)
{
	std::vector<char*> user_argv, project_argv, combined_argv;

	// Check the command line for the --noenv option.
	// Note that we have to handle 'ucg -- --noenv' properly, hence the one_past_end_or_double_dash search first.
	auto one_past_end_or_double_dash = std::find_if(argv, argv+argc, [](char *s){ return std::strcmp(s, "--") == 0; });
	auto noenv = std::count_if(argv, one_past_end_or_double_dash, [](char *s){ return std::strcmp(s, "--noenv") == 0; });

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

	// We have to handle User Defined Types and --TYPEs ourselves, before finally calling argp_parse().  argp doesn't
	// support dynamically added options, which we need for the --TYPEs that will be created by the User Defined Type
	// mechanism.
	HandleTYPELogic(&combined_argv);

	// Parse the combined list of arguments.
	argp_parse(&argp, combined_argv.size(), combined_argv.data(), 0, 0, this);

	//// Now set up defaults.

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

	// Search files/directories.
	if(m_paths.empty())
	{
		// Default to current directory.
		m_paths.push_back(".");
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
	std::fputs(argp_program_version, stream);

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
	std::fprintf(stream, " Name ($(CXX)): %s\n", g_cxx);
	std::fprintf(stream, " Version string: \"%s\"\n", g_cxx_version_str);

	//
	// libpcre info
	//
	std::fprintf(stream, "\nlibpcre info:\n");
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
	std::map<int, std::string> newline_desc { {10, "LF"}, {13, "CR"}, {3338, "CRLF"}, {-2, "ANYCRLF"}, {-1, "ANY"},
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
}

void ArgParse::PrintHelpTypes() const
{
	std::cout << "ucg recognizes the following file types:" << std::endl;
	std::cout << std::endl;
	m_type_manager.PrintTypesForHelp(std::cout);
	std::cout << std::endl;
}

void ArgParse::FindAndParseConfigFiles(std::vector<char*> */*global_argv*/, std::vector<char*> *user_argv, std::vector<char*> *project_argv)
{
	// Find and parse the global config file.
	/// @todo
	/// @note global_argv commented out above to avoid unused parameter warning.

	// Parse the user's config file.
	std::string homedir = GetUserHomeDir();
	if(!homedir.empty())
	{
		// See if we can open the user's .ucgrc file.
		homedir += "/.ucgrc";
		try
		{
			File home_file(homedir);

			if(home_file.size() == 0)
			{
				//std::clog << "INFO: config file \"" << homedir << "\" is zero-length." << std::endl;
			}
			else
			{
				auto vec_argv = ConvertRCFileToArgv(home_file);

				user_argv->insert(user_argv->end(), vec_argv.cbegin(), vec_argv.cend());
			}
		}
		catch(const FileException &e)
		{
			std::clog << "ucg: WARNING: " << e.what() << std::endl;
		}
		catch(const std::system_error &e)
		{
			if(e.code() != std::errc::no_such_file_or_directory)
			{
				std::clog << "ucg: WARNING: Couldn't open config file \"" << homedir << "\", error " << e.code() << " - " << e.code().message() << std::endl;
			}
			// Otherwise, the file just doesn't exist.
		}
	}

	// Find and parse the project config file.
	auto proj_rc_filename = GetProjectRCFilename();
	if(!proj_rc_filename.empty())
	{
		// We found it, see if we can open it.
		try
		{
			File proj_rc_file(proj_rc_filename);

			if(proj_rc_file.size() == 0)
			{
				//std::clog << "INFO: config file \"" << proj_rc_filename << "\" is zero-length." << std::endl;
			}
			else
			{
				//std::clog << "INFO: parsing config file \"" << proj_rc_filename << "\"." << std::endl;

				auto vec_argv = ConvertRCFileToArgv(proj_rc_file);

				project_argv->insert(project_argv->end(), vec_argv.cbegin(), vec_argv.cend());
			}
		}
		catch(const FileException &e)
		{
			std::clog << "ucg: WARNING: " << e.what() << std::endl;
		}
		catch(const std::system_error &e)
		{
			if(e.code() != std::errc::no_such_file_or_directory)
			{
				std::clog << "ucg: WARNING: Couldn't open config file \"" << homedir << "\", error " << e.code() << " - " << e.code().message() << std::endl;
			}
			// Otherwise, the file just doesn't exist.
		}
	}
}

std::string ArgParse::GetUserHomeDir() const
{
	std::string retval;

	// First try the $HOME environment variable.
	const char * home_path = getenv("HOME");

	if(home_path == nullptr)
	{
		// No HOME variable, check the user database.
		home_path = getpwuid(getuid())->pw_dir;
	}

	if(home_path != nullptr)
	{
		// Found user's HOME dir.
		retval = home_path;
	}

	return retval;
}

/**
 * Checks two file descriptors (file, dir, whatever) and checks if they are referring to the same entity.
 *
 * @param fd1
 * @param fd2
 * @return  true if fd1 and fd2 are fstat()able and refer to the same entity, false otherwise.
 */
static bool is_same_file(int fd1, int fd2)
{
	struct stat s1, s2;

	if(fstat(fd1, &s1) < 0)
	{
		return false;
	}
	if(fstat(fd2, &s2) < 0)
	{
		return false;
	}

	if(
		(s1.st_dev == s2.st_dev) // Same device
		&& (s1.st_ino == s2.st_ino) // Same inode
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string ArgParse::GetProjectRCFilename() const
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
	std::string homedirname = GetUserHomeDir();
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
#ifdef HAVE_GET_CURRENT_DIR_NAME
	char *original_cwd = get_current_dir_name();
#else
	char *original_cwd = getcwd(NULL, 0);
#endif

	//std::clog << "INFO: cwd = \"" << original_cwd << "\"" << std::endl;

	char *current_cwd = original_cwd;
	while((current_cwd != nullptr) && (current_cwd[0] != '.'))
	{
		// If we were able to get a file descriptor to $HOME above...
		if(home_fd != -1)
		{
			// ...check if this dir is the user's $HOME dir.
			int cwd_fd = open(current_cwd, O_RDONLY);
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
		auto test_rc_filename = std::string(current_cwd);
		if(*test_rc_filename.rbegin() != '/')
		{
			test_rc_filename += "/";
		}
		test_rc_filename += ".ucgrc";
		//std::clog << "INFO: checking for rc file \"" << test_rc_filename << "\"" << std::endl;
		int rc_file = open(test_rc_filename.c_str(), O_RDONLY);
		if(rc_file != -1)
		{
			// Found it.  Return its name.
			//std::clog << "INFO: found rc file \"" << test_rc_filename << "\"" << std::endl;
			retval = test_rc_filename;
			close(rc_file);
			break;
		}

		/// @note GRVS - get_current_dir_name() under Cygwin will currently return a DOS path if this is started
		///              under the Eclipse gdb.  This mostly doesn't cause problems, except for terminating the loop.
		///              The clause below after the || handles this.
		if((strlen(current_cwd) == 1) || (strlen(current_cwd) <= 4 && current_cwd[1] == ':'))
		{
			// We've hit the root and didn't find a config file.
			break;
		}

		// Go up one directory.
		current_cwd = dirname(current_cwd);
	}

	// Free the cwd string.
	free(original_cwd);

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
		auto arglen = strlen(*arg);
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
		if(m_type_manager.IsType(argtxt))
		{
			// Yes, replace it with something digestible by argp: --type=TYPE.
			std::string new_param("--type=" + argtxt);
			delete [] *arg;
			*arg = cpp_strdup(new_param.c_str());
		}

		// Is this a type specification of the form '--noTYPE'?
		else if(argtxt.compare(0, 2, "no") == 0 && m_type_manager.IsType(argtxt.substr(2)))
		{
			// Yes, replace it with something digestible by argp: --type=noTYPE.
			std::string new_param("--type=" + argtxt);
			delete [] *arg;
			*arg = cpp_strdup(new_param.c_str());
		}

		// Is this a type-add?
		else if(argtxt.compare(0, 9, "type-add=") == 0)
		{
			HandleTypeAddOrSet(argtxt);
		}

		// Is this a type-set?
		else if(argtxt.compare(0, 9, "type-set=") == 0)
		{
			m_type_manager.TypeDel(argtxt.substr(9));
			HandleTypeAddOrSet(argtxt);
		}

		// Is this a type-del?
		else if(argtxt.compare(0, 9, "type-del=") == 0)
		{
			// Tell the TypeManager to delete the type.
			m_type_manager.TypeDel(argtxt.substr(9));
		}
	}
}

static std::vector<std::string> split(const std::string &s, char delimiter)
{
	std::vector<std::string> retval;
	std::stringstream ss(s);
	std::string element;

	while(std::getline(ss, element, delimiter))
	{
		if(!element.empty())
		{
			retval.push_back(element);
		}
	}

	// This should allow for return value optimization.
	return retval;
}


void ArgParse::HandleTypeAddOrSet(const std::string& argtxt)
{
	std::string::size_type first_colon, second_colon;
	first_colon = argtxt.find_first_of(":");
	if(first_colon == std::string::npos)
	{
		// Malformed type spec.
		throw ArgParseException("Malformed type spec \"--" + argtxt + "\": Can't find first colon.");
	}
	second_colon = argtxt.find_first_of(":", first_colon+1);
	if(second_colon == std::string::npos)
	{
		// Malformed type spec.
		throw ArgParseException("Malformed type spec \"--" + argtxt + "\": Can't find second colon.");
	}
	if(second_colon <= first_colon+1)
	{
		// Malformed type spec, filter field is blank.
		throw ArgParseException("Malformed type spec \"--" + argtxt + "\": Filter field is empty.");
	}
	std::string type = argtxt.substr(9, first_colon-9);
	std::string filter = argtxt.substr(first_colon+1, second_colon-first_colon-1);
	std::string filter_args = argtxt.substr(second_colon+1);

	if(filter == "is")
	{
		// filter_args is a literal filename.
		m_type_manager.TypeAddIs(type, filter_args);
	}
	else if(filter == "ext")
	{
		// filter_args is a list of one or more comma-separated filename extensions.
		auto exts = split(filter_args,',');
		for(auto ext : exts)
		{
			m_type_manager.TypeAddExt(type, ext);
		}
	}
	else
	{
		// Unsupported filter type.
		throw ArgParseException("Unsupported filter type \"" + filter + "\" in type spec \"--" + argtxt + "\".");
	}
}
