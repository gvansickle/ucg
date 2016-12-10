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

#include <type_traits>

namespace std
{
#if !defined(__clang__) ///@todo For some reason I can't seem to figure out, The #if logic below
                        /// does not exclude this definition on clang.  Clang doesn't define __cpp_lib_make_unique,
                        /// and even though configure correctly detects HAVE_DECL_STD__MAKE_UNIQUE_INT_ == 1, my definition
                        /// still gets included, then conflicts with the definition clang does in fact have.
#if !defined(__cpp_lib_make_unique) && (HAVE_DECL_STD__MAKE_UNIQUE_INT_ == 0)  // C++14 feature.
/// Define our own make_unique<>() substitute.
/// @note SFINAE here to fail this for array types.
/// @note No need to check for __cpp_variadic_templates, it's C++11 and introduced in gcc 4.3.
template <typename T, typename... Args>
/// @todo old gcc doesn't like this: typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif
#endif
}

#endif /* SRC_FUTURE_MEMORY_HPP_ */
