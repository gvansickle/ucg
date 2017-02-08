/*
 * Copyright 2016-2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file microstring.hpp
 * A std:string-like class for strings short enough to fit into an unsigned literal int type.
 */

#ifndef SRC_LIBEXT_MICROSTRING_HPP_
#define SRC_LIBEXT_MICROSTRING_HPP_

#include <config.h>

#include <future/type_traits.hpp>
#include "integer.hpp"
#include "hints.hpp"


/**
 * Class for very short strings.  Basically a thin facade over a built-in integral type which allows very fast comparisons, copies, and moves.
 */
template <typename UnderlyingType>
class basic_microstring
{
public:

	/// @name Member Types
	/// @{
	/// Type returned by size(), length(), max_length().
	using size_type = typename uint_t<sizeof(UnderlyingType)*8>::type;
	using underlying_storage_type = UnderlyingType;
	/// @}

	// Default constructor.
	constexpr basic_microstring() = default;

	template <typename Stringlike>
	basic_microstring(const Stringlike &other) : basic_microstring(other.cbegin(), other.cend()) {};

	template <typename StringlikeConstIterator>
	basic_microstring(StringlikeConstIterator b, StringlikeConstIterator e,
			typename StringlikeConstIterator::iterator_category() = 0)
		: basic_microstring(&*b, &*e) {};

	basic_microstring(const char * __restrict__ cstr, size_type len) : basic_microstring(cstr, cstr+len) {};

	/**
	 * @title Dr. Substlove or: How I Learned To Stop Constructing and Love SFINAE
	 *
	 * Gentle Reader, prepare yourself for a journey to the center of C++'s mind.  Ok, so the basic_microstring<>
	 * class template is templated upon an underlying integral type.  Fast forward a few weeks: that means we
	 * need different non-default constructors for different sizes of the template.  E.g., for
	 * a basic_microstring<uint32_t>, we need a constructor which copies 4 bytes, but for a
	 * basic_microstring<uint64_t>, we need a constructor which copies 8 bytes.  "No problemo," I say,
	 * "that's what std::enable_if<> is for!"...
	 * ...and after I awakened, I realized that it was not that easy.  Here's some things which you can't do:
	 *
	 * - Overload a constructor with one with the same signature.  Obvious, of course, but it complicates other potential options.
	 * - Use std::enable_if to enable/disable the return type to cause substitution failure.  Because, of course, constructors
	 *   have no return type.
	 * - Use the class's template parameter directly to do any sort of SFINAE to eliminate member functions [@todo explain/ref]
	 * - Overload based solely on a template parameter's default value.
	 * - @todo etc...
	 *
	 * For SFINAE to work, the SF has to happen *in the S*[1], not outside it.  If you get compile errors where you think you should
	 * just be getting a template taken out of the overload resolution set, this may very well be what you're running into.
	 * [1] "Substitution Failure has to happen in the Substitution".
	 *
	 * The solution I use here is based somewhat on this SO post: @see http://stackoverflow.com/a/26949793/945838.  The key steps are:
	 * 1. Make the constructors member templates.
	 * 1a. Make the member template parameters default to the class's template parameter (i.e. typename T = UnderlyingType).
	 * 2. Add another parameter to the constructors.  This parameter will both cause the SFINAE to work *and* disambiguate
	 *    the otherwise-identical overloads.
	 * 2a. Give the param a default value of 0.
	 * 3. Use std::enable_if to specify the type of this parameter.  This is the key to the key here, and must be done very carefully.
	 *    Note the form here for a basic_microstring<uint32_t>:
	 *        basic_microstring(...params..., typename std::enable_if<sizeof(T)==4, T>::type = 0)
	 *    3a. The enable expression is here --------------------------^^^^^^^^^^^^  ^
	 *    3b. The Disambiguating Type(tm) is here ----------------------------------|
	 *        ...and is equivalent to the class's template type.  No new type needed to be introduced.
	 * 4. Let what's going on here sink in.  Basically, we've beaten C++11+'s best efforts to defeat us.  There are two cases:
	 * 4a. If, at instantiation time, sizeof(T) isn't 4, we get a normal substitution failure:
	 *        i.   basic_microstring(...params..., typename std::enable_if<sizeof(T)!=4, T>::type = 0)
	 *        ii.  basic_microstring(...params..., typename std::enable_if<false, T>::type = 0)
	 *        iii. basic_microstring(...params..., [nothing here, enable_if::type doesn't exist] = 0)
	 *        iv.  ==> Substitution Failure.
	 * 4b. But, if, at instantiation time, sizeof(T) is 4, a minor miracle happens:
	 *        i.   basic_microstring(...params..., typename std::enable_if<sizeof(T)==4, T>::type = 0)
	 *        ii.  basic_microstring(...params..., typename std::enable_if<true, T>::type = 0)
	 *        iii. basic_microstring(...params..., T = 0)
	 *        iv.  basic_microstring(...params..., uint32_t = 0)
	 *        v.   ==> Valid constructor prototype, disambiguated from any others which may be needed for different UnderlyingType's.
	 * 5. Bask in the knowledge that you've solved yet another of the world's problems.
	 */

	/**
	 * Constructor for 4-character microstrings.
	 * @param start
	 * @param end
	 * @param
	 */
	template <typename T = UnderlyingType>
	basic_microstring(const char * __restrict__ start, const char * __restrict__ end,
			typename std::enable_if<sizeof(T)==4, T>::type = 0)
	{
		static_assert(sizeof(T) == 4, "This constructor is for 4-byte microstrings only.");

		size_t num_chars = end - start;

		if(unlikely(num_chars > sizeof(underlying_storage_type)))
		{
			throw std::length_error("Length too long for a microstring");
		}

		assume(num_chars <= max_size());

		m_storage = static_cast<decltype(m_storage)>(0);
		uint8_t *storage_start = reinterpret_cast<uint8_t*>(&m_storage);

		// Copy the bytes.  Hopefully the compiler will optimize this loop.
		/// @note On Intel (little endian), we ultimately want to reverse the byte order.  We don't do it
		/// in this loop though, to give the compiler every opportunity to optimize it.
		for(uint8_t i=0; i<num_chars; i++)
		{
			storage_start[i] = start[i];
		}

		// Put the first character in the MSB, so that microstrings sort the same as a regular string.
		m_storage = host_to_be(m_storage);
	}

	/**
	 * Constructor for 8-character microstrings.
	 * @param start
	 * @param end
	 * @param
	 */
	template <typename T = UnderlyingType>
	basic_microstring(const char * __restrict__ start, const char * __restrict__ end,
			typename std::enable_if<sizeof(T)==8, T>::type = 0)
	{
		static_assert(sizeof(T) == 8, "This constructor is for 8-byte microstrings only.");

		size_t num_chars = end - start;
		if(unlikely(num_chars > sizeof(underlying_storage_type)))
		{
			throw std::length_error("Length too long for a microstring");
		}

		assume(num_chars <= max_size());

		m_storage = static_cast<decltype(m_storage)>(0);
		uint8_t *storage_start = reinterpret_cast<uint8_t*>(&m_storage);

		// Copy the bytes.  Hopefully the compiler will optimize this loop.
		/// @note On Intel (little endian), we ultimately want to reverse the byte order.  We don't do it
		/// in this loop though, to give the compiler every opportunity to optimize it.
		for(uint8_t i=0; i<num_chars; i++)
		{
			storage_start[i] = start[i];
		}

		// Put the first character in the MSB, so that microstrings sort the same as a regular string.
		m_storage = host_to_be(m_storage);
	}

	~basic_microstring() noexcept = default;

	/**
	 * Return the number of characters in the microstring.
	 *
	 * @return Length of string.
	 */
	inline size_type length() const noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return countnonzeros(m_storage);
	};

	constexpr inline size_type size() const noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return length();
	}

	/// Maximum size in characters.
	static constexpr inline size_type max_size() noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return sizeof(UnderlyingType);
	}

	constexpr inline bool empty() const noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return m_storage == 0;
	}

	constexpr inline bool operator<(const basic_microstring other) const noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return m_storage < other.m_storage;
	};

	constexpr inline bool operator==(const basic_microstring other) const noexcept ATTR_CONST ATTR_ARTIFICIAL
	{
		return m_storage == other.m_storage;
	};

	/// Implicitly convert to a std::string.
	inline operator std::string() const ATTR_ARTIFICIAL
	{
		underlying_storage_type tmp = host_to_be(m_storage);
		return std::string(reinterpret_cast<const char *>(&tmp), length());
	};

	underlying_storage_type urep() const noexcept { return m_storage; };

private:
	underlying_storage_type m_storage;  // No member initializer, would make the class non-trivial.
};

template < typename UnderlyingType >
constexpr bool operator==(basic_microstring<UnderlyingType> lhs, basic_microstring<UnderlyingType> rhs)
{
	return lhs == rhs;
}

using microstring = basic_microstring<uint64_t>;





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
static_assert(sizeof(microstring) == sizeof(microstring::underlying_storage_type), "microstring has a different size than its underlying storage");

static_assert(std::is_constructible<microstring, std::string>::value, "microstring is not constructible from std::string");
static_assert(std::is_trivially_destructible<microstring>::value, "microstring is not trivially destructible");

#endif /* SRC_LIBEXT_MICROSTRING_HPP_ */
