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

/** @file */

#ifndef SRC_FUTURE_SHARED_MUTEX_HPP_
#define SRC_FUTURE_SHARED_MUTEX_HPP_

#include <config.h>

#include <libext/static_diagnostics.hpp>

#if defined(__APPLE__) || defined(__apple_build_version__)
// We're on Mac OSX.
// Don't include the system <shared_mutex> header at all.
STATIC_MSG_WARN("Mac OSX detected, attempting to backfill <shared_mutex>");
#include <mutex>
#define HAVE_BROKEN_SHARED_MUTEX_HEADER
namespace std
{
		using shared_mutex = std::mutex;
		template <typename T>
		using shared_lock = std::unique_lock<T>;
}
#elif __has_include(<shared_mutex>) // C++14 feature.  Header check only, returns 1.
#	include <shared_mutex>
#	define HAVE_SHARED_MUTEX_HEADER 1
#	if __cpp_lib_shared_timed_mutex || defined(HAVE_STD__SHARED_TIMED_MUTEX) // C++14 feature which renamed std::shared_mutex to std::shared_timed_mutex.  <shared_mutex> and == 201402.

#		if __cpp_lib_shared_mutex || defined(HAVE_STD__SHARED_MUTEX) // C++17 feature which made the original C++14 std::shared_mutexes untimed and gave them their name back.  <shared_mutex> and == 201505
			// We have real std::shared_mutex's.  Nothing to do.
#		else
			// Use std::shared_timed_mutex's instead.
			namespace std
			{
				using shared_mutex = std::shared_timed_mutex;
			}
#		endif
#		if !defined(HAVE_SHARED_LOCK_SHARED_TIMED_MUTEX)
			namespace std
			{
				// Use std::unique_lock<> instead.
				STATIC_MSG_WARN("Backfilling shared_lock<shared_timed_mutex> with unique_lock<>.")
				template < typename T >
				using shared_lock = std::unique_lock<typename std::enable_if<std::is_same<T, std::shared_timed_mutex>::value>::type>;
			}
#		endif
#		if !defined(HAVE_SHARED_LOCK_SHARED_MUTEX)
			namespace std
			{
				// Use std::unique_lock<> instead.
				/// @todo From what I can tell, we shouldn't ever need this.  If we have the real std::shared_mutex, we should
				/// also have the real std::shared_lock<std::shared_mutex>.  If we only have std::shared_timed_mutex, the backfill
				/// in !HAVE_SHARED_LOCK_SHARED_TIMED_MUTEX above will be used regardless of whether we're trying to shared_lock
				/// a real std::shared_mutex or our backfilled version using std::shared_timed_mutex.
				//STATIC_MSG_WARN("Backfilling shared_lock<shared_mutex> with unique_lock<>.")
				//template < typename T >
				//using shared_lock = std::unique_lock<typename std::enable_if<std::is_same<T, std::shared_mutex>::value>::type>;
//#				define SHARED_LOCK_SHARED_MUTEX std::unique_lock<std::shared_mutex>;
			}
#		endif
#	else // Found the header, but it didn't define __cpp_lib_shared_timed_mutex.
		// Really shouldn't get here.
		// Guess what?  Clang++/libc++ on OS X (Xcode 8gm) does this: "error: 'shared_mutex' is unavailable: introduced in macOS 10.12".
		STATIC_MSG_WARN("Faulty header: <shared_mutex>");
#		define HAVE_BROKEN_SHARED_MUTEX_HEADER 1
	#endif
#else
	// Not even a <shared_mutex> header.
#	define NO_SHARED_MUTEX_HEADER 1
	STATIC_MSG_WARN("No <shared_mutex> header found")
#endif

#if defined(HAVE_BROKEN_SHARED_MUTEX_HEADER)

STATIC_MSG_WARN("Trying to build with broken <shared_mutex> header")

#elif !defined(HAVE_SHARED_MUTEX_HEADER) || defined(NO_SHARED_MUTEX_HEADER)

STATIC_MSG_WARN("Backfilling header <shared_mutex> from future");
#include <mutex>
namespace std
{
	using shared_mutex = std::mutex;
	using shared_timed_mutex = std::mutex;
	template < typename T >
	using shared_lock = std::unique_lock<T>;
}

#endif

#endif /* SRC_FUTURE_SHARED_MUTEX_HPP_ */
