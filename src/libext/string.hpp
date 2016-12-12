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

/** @file string.hpp
 * Collection of std::string-related utility functions which should probably already exist in the standard lib, but don't.
 */

#ifndef SRC_LIBEXT_STRING_HPP_
#define SRC_LIBEXT_STRING_HPP_

#include <config.h>

#include <future/string.hpp>
#include <future/type_traits.hpp>

#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>

#include "integer.hpp"
#include "hints.hpp"

#include "microstring.hpp"

/**
 * Splits the given string #s on the given #delimiter character.  Returns the resulting strings in a std::vector.
 *
 * @param s          String to split into substrings.
 * @param delimiter  Single delimiter char on which to split.
 * @return           The resulting strings.
 */
inline std::vector<std::string> split(const std::string &s, char delimiter)
{
	std::vector<std::string> retval;
	std::stringstream ss(s);
	std::string element;

	while(std::getline(ss, element, delimiter))
	{
		if(!element.empty())
		{
			retval.push_back(element);
		}
	}

	// This should allow for return value optimization.
	return retval;
}


/**
 * Joins the strings in #container_of_strings into a single string, optionally separated by #separator.
 *
 * @param container_of_strings
 * @param separator
 * @return
 */
template < typename ContainerType >
typename ContainerType::value_type join(const ContainerType& container_of_strings,
		const typename ContainerType::value_type separator = typename ContainerType::value_type())
{
	// In an attempt at increasing allocation efficiency, we scan the container twice: once to get the
	// size of the resulting string we'll end up with so we can reserve it, eliminating any reallocations.
	// The second scan then does the actual join of the contained strings into this preallocated space.

	size_t len = 0;
	size_t seplen = separator.length();
	std::for_each(container_of_strings.cbegin(), container_of_strings.cend(),
			[&len, seplen](const typename ContainerType::value_type &str){ len += seplen + str.length();});
	len += 1; // For any trailing '\0'.

	// Create the return value and allocate the space we'll need.
	typename ContainerType::value_type retval;
	retval.reserve(len);

	// Concatenate the strings together.
	for(auto entry : container_of_strings)
	{
		if(retval.length() != 0 && separator.length() != 0)
		{
			retval += separator;
		}
		retval += entry;
	}

	return retval;
}

/**
 * Overload of std::to_string() to make insertion of hex-formatted integral types into an ostream more possible.
 *
 * @param val
 * @param base
 * @return
 */
template < typename T >
std::string to_string(T val, std::ios_base & (*base)(std::ios_base&))
{
	std::ostringstream ss;

	ss << std::showbase << std::setw(sizeof(T)*2+2) << base << val;

	return ss.str();
}


/**
 * Wrapper for those (mostly C) functions which take a buffer which they will fill with a string of unknown
 * length, making you guess at how large a buffer you need.  This template papers over that nonsense.
 * Requires #func to have the following properties:
 * - Must take two parameters, the first being a "selector" of some type for selecting which string you want, the second
 *   being a pointer to a buffer.
 * - Must return the necessary length of the buffer when passed #tellmethelength as its second parameter.
 *
 * @param func
 * @param s
 * @return  A std:string containing the string #func put in the temporary buffer.
 */
template <typename CallableTwoParam, typename SelectorType, typename BuffType = char, BuffType *tellmethelength = nullptr>
std::string to_string(CallableTwoParam func, SelectorType s)
{
	auto len = func(s, tellmethelength);
	auto buffer = new BuffType[len+1];
	len = func(s, buffer);
	std::string retval {buffer};
	delete [] buffer;
	return retval;
}

#endif /* SRC_LIBEXT_STRING_HPP_ */
