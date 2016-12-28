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

#ifndef SRC_FUTURE_STRING_HPP_
#define SRC_FUTURE_STRING_HPP_

#include <config.h>
#include <string>
#include <sstream>

#include "type_traits.hpp"

#if !defined(HAVE_FUNC_STD__TO_STRING) || (HAVE_FUNC_STD__TO_STRING == 0)

// We have to backfill std::to_string() for broken C++11 std libs.  See e.g. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61580
// (fixed on gcc trunk 2015-11-13), https://sourceware.org/ml/cygwin/2015-01/msg00251.html.

namespace std
{

template <typename T>
string to_string(T value)
{
	static_assert(is_integral<T>::value, "Parameter passed to std::to_string() must be integral type.");
	stringstream temp_ss;
	temp_ss << value;
	return temp_ss.str();
}

} // namespace std

#endif

#endif /* SRC_FUTURE_STRING_HPP_ */
