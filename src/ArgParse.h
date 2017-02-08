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

/** @file */

#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include <config.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <cstdio>


class TypeManager;
class File;


/**
 * ArgParse will throw this if it runs into trouble parsing the rc files or command line.
 */
struct ArgParseException : public std::runtime_error
{
	ArgParseException(const std::string &message) : std::runtime_error(message) {};
};


/**
 * Command-line and config file parser.
 */
class ArgParse
{
public:
	ArgParse(TypeManager &tm);
	~ArgParse();

	/**
	 * Parse the given command line.
	 *
	 * @param argc  The argc from main().
	 * @param argv  The argv from main().
	 */
	void Parse(int argc, char **argv);

	/**
	 * Argp overload of argp_program_version_hook.
	 *
	 * @note This would preferably be private, and maybe non-static, but argp needs it to be statically overloaded (global fn ptr).
	 *
	 * @param stream  Output stream from argp where we are to send the version info.
	 */
	static void PrintVersionText(std::FILE *stream);

private:

	/// Reference to the TypeManager passed into the constructor.
	TypeManager &m_type_manager;

	/// The argp struct we'll pass to arg_parse() from the GNU argp library.
	static struct argp argp;

	/// The callback which receives the parsed options.
	//static error_t parse_opt (int key, char *arg, struct argp_state *state);

	void PrintHelpTypes() const;

	std::string GetProjectRCFilename() const;

	/**
	 * Pre-parse the given config (.ucgrc) file, removing comments and returning a vector of
	 * command-line parameters as char *'s.
	 *
	 * @note The returned char *'s must be delete[]ed or they will leak.
	 *
	 * @param f  The config File.
	 * @return  vector<char*> of command-line params.
	 */
	std::vector<char *> ConvertRCFileToArgv(const File &f);

	void FindAndParseConfigFiles(std::vector<char*> *global_argv, std::vector<char*> *user_argv, std::vector<char*> *project_argv);

	void HandleTYPELogic(std::vector<char *> *v);

	/// If true, ArgParse won't look for or use $HOME/.ucgrc.
	/// Used for testing.
	bool m_test_noenv_user { false };

	/// true if smart-case should be used.
	/// This isn't public because the logic which makes the final decision on whether to ultimately match
	/// case-insensitively or not is internal to this class, and its decision is exposed as m_ignore_case.
	bool m_smart_case { true };

public:

	/// @name The Parsed Options
	/// These are the options parsed from the config files and command line.  They're public:,
	/// which is a deliberate design decision to allow easy read access to the resulting parsed info.  If I
	/// get ambitious, it might make sense to factor these into a separate struct that gets passed around instead.
	///@{

	/// The regex to be matched.
	std::string m_pattern;

	/// true if the case of PATTERN should be ignored.
	bool m_ignore_case { false };

	/// true if PATTERN should only match whole words.
	bool m_word_regexp { false };

	/// true if PATTERN should be treated as literal chars (i.e. not a regex).
	bool m_pattern_is_literal { false };

	/// true if we should print the column of the first match after the line number.
	bool m_column { false };

	/// The file and directory paths given on the command line.
	std::vector<std::string> m_paths;

	/// Directory names to be excluded from the search.
	std::set<std::string> m_excludes;

	/// Number of FileScanner threads to use.
	int m_jobs { 0 };

	/// Number of Globber threads to use.
	int m_dirjobs { 0 };

	/// Whether to use color output or not.
	/// both false == not specified on command line.
	bool m_color { false };
	bool m_nocolor { false };

	/// Whether to recurse into subdirectories or not.
	bool m_recurse { true };

	bool m_follow_symlinks { false };

	bool m_use_mmap { false };

	///@}
};

#endif /* ARGPARSE_H_ */
