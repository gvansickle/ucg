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
 * Class for very short strings.  Basically a thin facade over a built-in integral type which allows very fast comparisons, copies, and moves.
 */
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
		if(__builtin_expect(num_chars > sizeof(underlying_storage_type), 0))
		{
			throw std::length_error("Length too long for a microstring");
		}

		// Put the first character in the MSB, so that microstrings sort the same as a regular string.
		m_storage = 0;
		if(num_chars > 0)
		{
			m_storage = (static_cast<underlying_storage_type>(*start) << (8*3));
		}
		if(num_chars > 1)
		{
			++start;
			m_storage |= (static_cast<underlying_storage_type>(*start) << (8*2));
		}
		if(num_chars > 2)
		{
			++start;
			m_storage |= (static_cast<underlying_storage_type>(*start) << (8*1));
		}
		if(num_chars > 3)
		{
			++start;
			m_storage |= static_cast<underlying_storage_type>(*start);
		}
	}

	~microstring() noexcept = default;

	size_t length() const noexcept
	{
		auto tmp = m_storage;

		// If we're little-endian, swap the bytes of the tmp var.
		/// @todo Check for little endianness.
		tmp = __builtin_bswap32(tmp);

		auto ptr = reinterpret_cast<const char *>(&tmp);

		return strnlen(ptr, 4);
	};

	bool operator <(const microstring other) const noexcept { return m_storage < other.m_storage; };

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

#ifdef HAVE_IS_TRIVIAL
static_assert(std::is_trivial<microstring>::value, "microstring is not trivial");
#endif
#ifdef HAVE_IS_TRIVIALLY_COPYABLE // gcc/libstdc++ 4.8.4 doesn't have these for some reason.
static_assert(std::is_trivially_copyable<microstring>::value, "microstring is not trivially copyable");
static_assert(std::is_trivially_copy_constructible<microstring>::value, "microstring is not trivially copy-constructible");
static_assert(std::is_trivially_move_constructible<microstring>::value, "microstring is not trivially move-constructible");
static_assert(std::is_trivially_assignable<microstring, microstring>::value, "microstring is not trivially assignable to itself");
static_assert(std::is_trivially_copy_assignable<microstring>::value, "microstring is not trivially copy-assignable");
static_assert(std::is_trivially_move_assignable<microstring>::value, "microstring is not trivially move-assignable");
#endif
static_assert(sizeof(microstring) == sizeof(uint32_t), "microstring has a different size than its underlying storage");

static_assert(std::is_constructible<microstring, std::string>::value, "microstring is not constructible from std::string");
static_assert(std::is_trivially_destructible<microstring>::value, "microstring is not trivially destructible");


#if !defined(HAVE_DECL_STD__TO_STRING) || (HAVE_DECL_STD__TO_STRING == 0)

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

#endif /* SRC_LIBEXT_STRING_HPP_ */
