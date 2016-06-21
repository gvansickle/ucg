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

#ifndef SRC_LIBEXT_FILESYSTEM_HPP_
#define SRC_LIBEXT_FILESYSTEM_HPP_

#include <config.h>

#include <sys/types.h> // for dev_t, ino_t

#include "integer.hpp"

using dev_ino_pair_type = uint_t<(sizeof(dev_t)+sizeof(ino_t))*8>::fast;

struct dev_ino_pair
{
	dev_ino_pair(dev_t d, ino_t i) noexcept { m_val = d, m_val <<= sizeof(ino_t)*8, m_val |= i; };

	dev_ino_pair_type m_val;
};



#endif /* SRC_LIBEXT_FILESYSTEM_HPP_ */
