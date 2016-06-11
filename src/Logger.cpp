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
 * @file A basic multithreaded logging facility.
 */

#include <config.h>

#include "Logger.h"

#include <string>
#include <thread>

#if defined(CXX11_THREADS_ARE_PTHREADS)
#	if defined(HAVE_PTHREAD)
#		include <pthread.h>
#		if defined(INCLUDE_PTHREAD_NP)
#			include <pthread_np.h>
#		endif // INCLUDE_PTHREAD_NP
#	endif // HAVE_PTHREADS
#endif // CXX11_THREADS_ARE_PTHREADS

std::string Logger::m_program_invocation_name;
std::string Logger::m_program_invocation_short_name;

/// Mutex for serializing writes to cerr.
std::mutex Logger::m_cerr_mutex;

#if !defined(HAVE_NO_THREAD_LOCAL_SUPPORT)
// Apple specifically disables C++11 thread_local support in the clang they ship:
// http://stackoverflow.com/questions/28094794/why-does-apple-clang-disallow-c11-thread-local-when-official-clang-supports?lq=1
/// A thread-local copy of the thread's name, so we don't have to call into the non-standardized pthread_getname_np() API to get it.
thread_local std::string thread_name {"UNKNOWN"};
#endif


void set_thread_name(const std::string &name)
{
	// pthreads can only handle thread names of 15 chars + \0.  We'll use the same limit for logging.
	std::string name_15plusnull {(name.length() < 15) ? name : name.substr(0, 15)};

#if defined(CXX11_THREADS_ARE_PTHREADS) && HAVE_PTHREAD_SETNAME_SUPPORT
	// Set the name of the pthread so we can see it in the debugger.
	M_pthread_setname_np(name_15plusnull.data());
#endif

#if !defined(HAVE_NO_THREAD_LOCAL_SUPPORT)
	thread_name = name_15plusnull;
#endif
}

std::string get_thread_name()
{
#if !defined(HAVE_NO_THREAD_LOCAL_SUPPORT)
	return thread_name;
#else
	// This is OSX.  Pull the name out of pthreads.
	char buffer[16];
	pthread_getname_np(pthread_self(), buffer, 16);
	return std::string(buffer, 16);
#endif
}
