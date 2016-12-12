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

/** @file Dummy cpp file to get this otherwise header-only lib to build portably. */

#include <config.h>
#include "shared_mutex.hpp"
#include <static_diagnostics.hpp>

#include "memory.hpp"


#include "memory.hpp"

const char *link_me = "dummy";

// Use this file for printing out some info at compile time regarding our compile-time environment.
#ifdef __SSE2__
STATIC_MSG("Have SSE2")
#endif
#ifdef __SSE4_2__
STATIC_MSG("Have SSE4_2")
#endif
#ifdef __POPCNT__
STATIC_MSG("Have POPCNT")
#endif

#if defined(__has_include)
STATIC_MSG("__has_include is defined.")
#else
STATIC_MSG_WARN("__has_include is not defined.")
#endif

#if defined(__cpp_lib_make_unique)
STATIC_MSG("__cpp_lib_make_unique is defined.")
#else
STATIC_MSG_WARN("__cpp_lib_make_unique is not defined.")
#endif

///
#if __cpp_lib_shared_timed_mutex
STATIC_MSG("__cpp_lib_shared_timed_mutex is defined")
#else
STATIC_MSG_WARN("__cpp_lib_shared_timed_mutex not defined")
#endif
#if __cpp_lib_shared_mutex
STATIC_MSG("__cpp_lib_shared_mutex is defined")
#else
STATIC_MSG_WARN("__cpp_lib_shared_mutex not defined")
#endif

#if __has_include(<shared_mutex>)
STATIC_MSG("Have __has_include(<shared_mutex>)")
#else
STATIC_MSG("No __has_include(<shared_mutex>)")
#endif

#ifndef _GLIBCXX_USE_C99_STDINT_TR1
STATIC_MSG_WARN("No _GLIBCXX_USE_C99_STDINT_TR1")
#endif

#ifndef _GLIBCXX_HAS_GTHREADS
STATIC_MSG_WARN("No _GLIBCXX_HAS_GTHREADS")
#endif


#if 0 /// @note Put this in to see at compile time what types are really being used for the shared locks.
std::shared_mutex i;
std::shared_timed_mutex j;
std::shared_lock<std::shared_timed_mutex> k;
std::shared_lock<std::shared_mutex> l;
auto testVar = std::make_tuple(i, j, k, l);
static_assert(decltype(testVar)::dummy_error, "TYPES" );
#endif

