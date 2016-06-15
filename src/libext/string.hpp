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

#ifndef SRC_LIBEXT_STRING_HPP_
#define SRC_LIBEXT_STRING_HPP_

#include <config.h>

#include <type_traits>
#include <cstring>
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


class microstring
{
public:

	using underlying_storage_type = uint32_t;

	microstring() = default;
	microstring(const std::string &other) : microstring(other.cbegin(), other.cend()) {};
	microstring(std::string::const_iterator b, std::string::const_iterator e) : microstring(&*b, &*e) {};

	microstring(const char * __restrict__ start, const char * __restrict__ end)
	{
		size_t num_chars = end - start;
		if(num_chars > sizeof(underlying_storage_type))
		{
			throw std::length_error("Length too long for a microstring");
		}

		// Put the first character in the MSB, so that microstrings sort the same as a regular string.
		m_storage = 0;
		while(num_chars > 0)
		{
			m_storage |= (static_cast<underlying_storage_type>(*start) << (8*num_chars));
			++start;
			--num_chars;
		}
	}

	~microstring() noexcept = default;

	size_t length() const noexcept
	{
		auto tmp = m_storage;
		auto ptr = reinterpret_cast<const char *>(tmp);

		// If we're little-endian, swap the bytes of the tmp var.
		/// @todo Check for little endianness.
		tmp = __builtin_bswap32(tmp);

		return strnlen(ptr, 4);
	};

	bool operator <(const microstring other) const { return m_storage < other.m_storage; };

	/// Implicitly convert to a std::string.
	operator std::string() const
	{
		underlying_storage_type tmp = __builtin_bswap32(m_storage);
		return std::string(reinterpret_cast<const char *>(&tmp), length());
	};

	underlying_storage_type urep() const noexcept { return m_storage; };

private:
	underlying_storage_type m_storage;
};

static_assert(std::is_trivial<microstring>::value, "microstring is not trivial");
static_assert(std::is_trivially_copyable<microstring>::value, "microstring is not trivially copyable");

#endif /* SRC_LIBEXT_STRING_HPP_ */
