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

#ifndef SRC_LIBEXT_STRING_HPP_
#define SRC_LIBEXT_STRING_HPP_

#include <string>
#include <sstream>
#include <vector>

/**
 * Splits the given string #s on the given #delimiter character.  Returns the resulting strings in a std::vector.
 *
 * @param s
 * @param delimiter
 * @return
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


#endif /* SRC_LIBEXT_STRING_HPP_ */
