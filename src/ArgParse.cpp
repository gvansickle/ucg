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

#include "ArgParse.h"

#include <algorithm>
#include <vector>
#include <string>
#include <locale>
#include <thread>
#include <iostream>
#include <system_error>

#include <argp.h>
#include <cstdlib>
#include <cstring>

#include <pwd.h> // for GetUserHomeDir()-->getpwuid().
#include <fcntl.h>
#include <unistd.h> // for GetUserHomeDir()-->getuid().
#include <sys/stat.h>
#include <libgen.h>   // Don't know where "libgen" comes from, but this is where POSIX says dirname() and basename() are declared.

#include "config.h"
#include "TypeManager.h"
#include "File.h"

// Not static, argp.h externs this.
const char *argp_program_version = PACKAGE_STRING "\n"
	"Copyright (C) 2015 Gary R. Van Sickle.\n"
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

/// @name Keys for options without short-options.
///@{
#define OPT_COLOR          1
#define OPT_NOCOLOR        2
#define OPT_IGNORE_DIR     3
#define OPT_NOIGNORE_DIR     4
#define OPT_TYPE			5
#define OPT_NOENV			6
///@}

/// Status code to use for a bad parameter which terminates the program via argp_failure().
/// Ack returns 255 in this case, so we'll use that instead of BSD's EX_USAGE, which is 64.
#define STATUS_EX_USAGE 255

// Not static, argp.h externs this.
error_t argp_err_exit_status = STATUS_EX_USAGE;

static struct argp_option options[] = {
		{0,0,0,0, "Searching:" },
		{"ignore-case", 'i', 0,	0,	"Ignore case distinctions in PATTERN."},
		{"word-regexp", 'w', 0, 0, "PATTERN must match a complete word."},
		{"literal", 'Q', 0, 0, "Treat all characters in PATTERN as literal."},
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
		{"type", OPT_TYPE, "[no]TYPE", 0, "Include only [exclude all] TYPE files."},
		{0,0,0,0, "Miscellaneous:" },
		{"noenv", OPT_NOENV, 0, 0, "Ignore .ucgrc files."},
		{"jobs",  'j', "NUM_JOBS",      0,  "Number of scanner jobs (std::thread<>s) to use." },
		{ 0 }
	};

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
	case OPT_NOENV:
		// The --noenv option is handled specially outside of the argp parser.
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
		break;
	case OPT_NOCOLOR:
		arguments->m_color = false;
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

struct argp ArgParse::argp = { options, ArgParse::parse_opt, args_doc, doc };


ArgParse::ArgParse(TypeManager &type_manager)
	: m_type_manager(type_manager)
{

}

ArgParse::~ArgParse()
{
	// TODO Auto-generated destructor stub
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
	auto noenv = std::count_if(argv, argv+argc, [](char *s){ return strcmp(s, "--noenv") == 0; });

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

void ArgParse::FindAndParseConfigFiles(std::vector<char*> *global_argv, std::vector<char*> *user_argv, std::vector<char*> *project_argv)
{
	// Find and parse the global config file.
	/// @todo

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
		catch(const std::system_error &e)
		{
			if(e.code() != std::errc::no_such_file_or_directory)
			{
				std::clog << "WARNING: Couldn't open config file \"" << homedir << "\", error " << e.code() << " - " << e.code().message() << std::endl;
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
		catch(const std::system_error &e)
		{
			//std::clog << "INFO: Couldn't open config file \"" << proj_rc_filename << "\", error " << e.code() << " - " << e.code().message() << std::endl;
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
	auto homedirname = GetUserHomeDir();
	int home_fd = -1;
	if(!homedirname.empty())
	{
		home_fd = open(homedirname.c_str(), O_RDONLY | O_DIRECTORY);
	}

	// Get the current working directory's absolute pathname.
	char *original_cwd = get_current_dir_name();

	//std::clog << "INFO: cwd = \"" << original_cwd << "\"" << std::endl;

	auto current_cwd = original_cwd;
	while((current_cwd != nullptr) && (current_cwd[0] != '.'))
	{
		// See if this is the user's $HOME dir.
		auto cwd_fd = open(current_cwd, O_RDONLY | O_DIRECTORY);
		if(is_same_file(cwd_fd, home_fd))
		{
			// We've hit the user's home directory without finding a config file.
			close(cwd_fd);
			break;
		}
		close(cwd_fd);

		// Try to open the config file.
		auto test_rc_filename = std::string(current_cwd);
		if(*test_rc_filename.rbegin() != '/')
		{
			test_rc_filename += "/";
		}
		test_rc_filename += ".ucgrc";
		//std::clog << "INFO: checking for rc file \"" << test_rc_filename << "\"" << std::endl;
		auto rc_file = open(test_rc_filename.c_str(), O_RDONLY);
		if(rc_file != -1)
		{
			// Found it.  Return its name.
			//std::clog << "INFO: found rc file \"" << test_rc_filename << "\"" << std::endl;
			retval = test_rc_filename;
			close(rc_file);
			break;
		}

		if(strlen(current_cwd) == 1)
		{
			// We've hit the root and didn't find a config file.
			break;
		}

		// Go up one directory.
		current_cwd = dirname(current_cwd);
	}

	// Free the cwd string.
	free(original_cwd);

	// Close the homedir we opened above.
	close(home_fd);

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
