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

#ifndef SRC_FUTURE_MEMORY_HPP_
#define SRC_FUTURE_MEMORY_HPP_

#include <config.h>

#include <memory>

#include "type_traits.hpp"

namespace std
{

#if !defined(__cpp_lib_make_unique) && !HAVE_DECL_STD__MAKE_UNIQUE_INT_  // C++14 feature.
/// Define our own make_unique<>() substitute.
/// @note SFINAE here to fail this for array types.
/// @note No need to check for __cpp_variadic_templates, it's C++11 and introduced in gcc 4.3.
template <typename T, typename... Args>
//typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

}

#endif /* SRC_FUTURE_MEMORY_HPP_ */
