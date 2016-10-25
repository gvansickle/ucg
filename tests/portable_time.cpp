/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file
 * In the 21st century, this program should not be necessary.  But it is, since we cannot rely on
 * the POSIX 'time(1)' command to be present on POSIX systems.  Even if it is present on a POSIX system,
 * it may not support the POSIX '-p' option.  Or the separate executable may not be present, but rather is provided as
 * a shell builtin, which causes problems if you try to put it in a shell variable.
 */

#include <iostream>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

// Per POSIX.1-2008, not all implementations have WCONTINUED.
#ifndef WCONTINUED
#define WCONTINUED 0
#endif

/// The parent's environment, which will be passed to the child.
/// This is a global, declared in unistd.h.
extern char **environ;

inline void pexit(const char *msg, int alt_errno = 0)
{
	if(alt_errno != 0)
	{
		errno = alt_errno;
	}
	perror(msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int opt;

	// Parse any of our own options.
	// The leading '+' is to force glibc's getopt() to parse options in order.
	while ((opt = getopt(argc, argv, "+p")) != -1)
	{
		switch (opt)
		{
		case 'p':
			// Accept the '-p' (POSIX-style time), but the output is already POSIX, so do nothing.
			break;
		case '?':
			// Unrecognized option.
			return EXIT_FAILURE;
		}
	}

	// Set up for timing the program.
	int retval;
	std::chrono::time_point<std::chrono::steady_clock> start, end;

	start = std::chrono::steady_clock::now();

	if(optind < argc)
	{
		// We parsed all the options, and the rest of the command line is the program to time.

		pid_t child_pid;
		char **child_argv = &argv[optind];
		int spawn_status;
		int child_status;

		// Spawn the child process.
		spawn_status = posix_spawnp(&child_pid, child_argv[0], nullptr, nullptr, child_argv, environ);
		if(spawn_status != 0)
		{
			pexit("posix_spawn", spawn_status);
		}

		// Wait until the child process terminates.
		do
		{
			spawn_status = waitpid(child_pid, &child_status, WUNTRACED | WCONTINUED);
			if(spawn_status == -1)
			{
				pexit("waitpid");
			}
		} while(!WIFEXITED(child_status) && !WIFSIGNALED(child_status));

		retval = WEXITSTATUS(child_status);
	}
	else
	{
		// No program to time was given.
		retval = 0;
	}

	end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;

	// Send the time to stderr.
	std::cerr << "real " << elapsed_seconds.count() << "\n";
	/// @todo System and User times not yet implemented.
	std::cerr << "user 0.00\nsys 0.00\n";

	return retval;
}


