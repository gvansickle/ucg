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

#if defined(__cpp_lib_make_unique)  // C++14 feature.
// We have a std::make_unique<>().
#else
/// Define our own make_unique<>() substitute.
/// @note SFINAE here to fail this for array types.
template <typename T, typename... Args>
typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
	make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

}

#endif /* SRC_FUTURE_MEMORY_HPP_ */
