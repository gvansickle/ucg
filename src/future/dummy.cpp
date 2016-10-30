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

#include <static_diagnostics.hpp>

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

