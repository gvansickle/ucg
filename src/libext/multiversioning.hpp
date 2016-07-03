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

/** @file  Portable Function multiversioning functionality. */

#ifndef SRC_LIBEXT_MULTIVERSIONING_HPP_
#define SRC_LIBEXT_MULTIVERSIONING_HPP_

/// @name Preprocessor token appending helpers.
/// These two combined allow the expansion of two preprocessor tokens to be token-pasted.
///@{
#define TOKEN_APPEND_HELPER(tok1, ...) tok1 ## __VA_ARGS__
#define TOKEN_APPEND(tok1, ...) TOKEN_APPEND_HELPER(tok1, __VA_ARGS__)
///@}

/// @name MULTIVERSION_DECORATOR_<FEATURE> function definition decorators
///@{
#if defined(__SSE4_2__) && __SSE4_2__==1
#define MULTIVERSION_DECORATOR_SSE4_2	_sse4_2
#endif
#if defined(__POPCNT__) && __POPCNT__==1
#define MULTIVERSION_DECORATOR_POPCNT	_popcnt
#else
#define MULTIVERSION_DECORATOR_POPCNT	_no_popcnt
#endif
///@}

#define MULTIVERSION(funcname) TOKEN_APPEND(TOKEN_APPEND(funcname, MULTIVERSION_DECORATOR_SSE4_2), MULTIVERSION_DECORATOR_POPCNT)

/// This macro would be what expands to the multiversion function definition under gcc.
/// So something like this in a .c/.cpp file:
///   void *memcpy(void *, const void *, size_t) __attribute__ ((ifunc ("resolve_memcpy")));
/// Note that is not a declaration, but a definition.
/// Anyway, since we can't rely on gcc's ifunc() functionality, we'll macro up something similar.
#define MULTIVERSION_DEF(funcname, func_type_def, ifunc_resolver_name) \
	/* ifunc()-like resolver for funcname(). */ \
	extern "C"	void * ifunc_resolver_name (void) ; \
	using funcname ## _type = decltype(func_type_def); \
	funcname ## _type funcname = reinterpret_cast<funcname ## _type>(:: ifunc_resolver_name ()));


#endif /* SRC_LIBEXT_MULTIVERSIONING_HPP_ */
