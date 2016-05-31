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

#include "Logger.h"

#if defined(CXX11_THREADS_ARE_PTHREADS)
#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif
#endif // CXX11_THREADS_ARE_PTHREADS

std::string Logger::m_program_invocation_name;
std::string Logger::m_program_invocation_short_name;

Logger::Logger()
{
	// TODO Auto-generated constructor stub

}


void set_thread_name(const std::thread &thread, const std::string &name)
{
#if defined(CXX11_THREADS_ARE_PTHREADS) && HAVE_DECL_PTHREAD_SETNAME_NP
	// Set the name of the pthread so we can see it in the debugger.

#endif
}
