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

#include <libext/static_diagnostics.hpp>

#if __has_include(<shared_mutex>)
#include <shared_mutex> // C++14 feature.  Header check only.

	#if __cpp_lib_shared_timed_mutex // C++14 feature which renamed std::shared_mutex to std::shared_timed_mutex.  <shared_mutex> and == 201402.

		#if __cpp_lib_shared_mutex // C++17 feature which made the original C++14 std::shared_mutexes untimed and gave them their name back.  <shared_mutex> and == 201505
			// We have real std::shared_mutex's.  Nothing to do.
		#else
			// Use std::shared_timed_mutex's instead.
			namespace std
			{
				using shared_mutex = std::shared_timed_mutex;
			}
		#endif
	#else
		// Really shouldn't get here.  Means we have <shared_mutex>, but not the required def.
		STATIC_MSG_WARN("Faulty header: <shared_mutex>");
	#endif
#else
	// Not even a <shared_mutex> header.
#include <mutex>
	namespace std
	{
		using shared_mutex = decltype(std::mutex);
		using shared_timed_mutex = decltype(std::mutex);
		using shared_lock = decltype(std::unique_lock);
	}
#endif


#endif /* SRC_FUTURE_SHARED_MUTEX_HPP_ */
