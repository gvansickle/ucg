/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of CoFlo.
 *
 * CoFlo is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * CoFlo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * CoFlo.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUILD_INFO_H
#define BUILD_INFO_H

/** @file build_info.h
 * Header for the built source file build_info.cpp.  Declarations to strings determined at make-time,
 * e.g. repo info (git describe), compiler name and version info, etc.
 */

/// Output of "git describe" in the top_srcdir at make time.  This will look like either:
/// - "0.2.0-4-g6fe34f4-dirty", with "-dirty" only appearing if there are uncommitted changes in the workspace, or
/// - "unknown" if git didn't exist on the machine or it wasn't a git repo.
extern const char *g_git_describe;

/// The C++ compiler used to build, i.e. the contents of make variable $CXX.
/// This will be "gcc", "clang++", etc., the name of the actual binary.
extern const char *g_cxx;

/// The C++ compiler's version string.
/// This is the first line of the output of "$(CXX) --version".  It will look something like:
/// - "g++ (GCC) 4.9.3"
/// - "clang version 3.5.2 (tags/RELEASE_352/final)"
extern const char *g_cxx_version_str;

#endif // BUILD_INFO_H
