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

/** @file string_view.hpp
 * Portability header which includes a std::string_view class from the future.
 */

#ifndef SRC_FUTURE_STRING_VIEW_HPP_
#define SRC_FUTURE_STRING_VIEW_HPP_

#include <config.h>

#if	__has_include(<string_view>)
// We have the real "Library Fundamentals V1 TS Components" std::string_view slated for C++17 (plus __has_include() works).
#include <string_view>
#elif defined(HAVE_STD__EXPERIMENTAL__STRING_VIEW)
// We have a pre-standard experimental version, use that.
#include <experimental/string_view>
namespace std
{
	using string_view = std::experimental::string_view;
}
#else
// Can't find anything.  Fake what we use with std::string.
#include <string>
namespace std
{
	using string_view = std::string;
}
#endif


#endif /* SRC_FUTURE_STRING_VIEW_HPP_ */
