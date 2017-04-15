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

/** @file  Portable CPUID-related functionality. */

#ifndef SRC_LIBEXT_CPUIDEX_HPP_
#define SRC_LIBEXT_CPUIDEX_HPP_

#include <config.h>

/// @name x86-64 extensions
/// @{
bool sys_has_sse2() noexcept;
bool sys_has_sse4_2() noexcept;
bool sys_has_popcnt() noexcept;
bool sys_has_avx() noexcept;
/// @}

#endif /* SRC_LIBEXT_CPUIDEX_HPP_ */
