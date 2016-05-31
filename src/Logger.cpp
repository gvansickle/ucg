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

#include <config.h>

#include "Logger.h"

#include <string>
#include <thread>

#if defined(CXX11_THREADS_ARE_PTHREADS)
#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif // HAVE_PTHREADS
#endif // CXX11_THREADS_ARE_PTHREADS

std::string Logger::m_program_invocation_name;
std::string Logger::m_program_invocation_short_name;

thread_local std::string thread_name {"UNKNOWN"};

Logger::Logger()
{
	// TODO Auto-generated constructor stub

}


void set_thread_name(const std::string &name)
{
	// pthreads can only handle thread names of 15 chars + \0.  We'll use the same limit for logging.
	std::string name_15plusnull {(name.length() < 15) ? name : name.substr(0, 15)};

#if defined(CXX11_THREADS_ARE_PTHREADS) && HAVE_DECL_PTHREAD_SETNAME_NP
	// Set the name of the pthread so we can see it in the debugger.
	pthread_setname_np(pthread_self(), name_15plusnull.data());
#endif

	thread_name = name_15plusnull;
}

std::string get_thread_name()
{
	return thread_name;
}
