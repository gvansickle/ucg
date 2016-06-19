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

#ifndef SRC_LIBEXT_INTEGER_HPP_
#define SRC_LIBEXT_INTEGER_HPP_

#include <config.h>

#include <cstdint>


// Boost has a template of this nature, but of course more complete.
template <unsigned char NumBits>
struct uint_t
{
	static_assert(NumBits <= 128, "NumBits > 128 not supported");
	using fast = typename uint_t<NumBits+1>::fast;
};
template<> struct uint_t<128> { using fast = unsigned __int128; };
template<> struct uint_t<64> { using fast = uint_fast64_t; };
template<> struct uint_t<32> { using fast = uint_fast32_t; };



#endif /* SRC_LIBEXT_INTEGER_HPP_ */
