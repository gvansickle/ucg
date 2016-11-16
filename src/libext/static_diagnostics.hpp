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

/** @file  Facilities for printing diagnostic info at compile time. */

#ifndef SRC_LIBEXT_STATIC_DIAGNOSTICS_HPP_
#define SRC_LIBEXT_STATIC_DIAGNOSTICS_HPP_

#include <config.h>

/// @name Preprocessor token appending helpers.
/// These two combined allow the expansion of two preprocessor tokens to be token-pasted.
///@{
#define TOKEN_APPEND_HELPER(tok1, ...) tok1 ## __VA_ARGS__
#define TOKEN_APPEND(tok1, ...) TOKEN_APPEND_HELPER(tok1, __VA_ARGS__)
///@}

/// @todo Not sure if we need these, we probably do.
//#define EXPAND_MACRO_HELPER(x) #x
//#define EXPAND_MACRO(p) EXPAND_MACRO_HELPER(p)


/// For passing in arguments to macros which may contain commas.
#define SINGLE_ARG(...)  __VA_ARGS__

#define PRAGMA_HELPER(t)	_Pragma (#t)

/// @todo Make this use GCC warning if no message support.
#define STATIC_MSG(m)       PRAGMA_HELPER(message #m)
#define STATIC_MSG_WARN(m)  PRAGMA_HELPER(GCC warning #m)

#endif /* SRC_LIBEXT_STATIC_DIAGNOSTICS_HPP_ */
