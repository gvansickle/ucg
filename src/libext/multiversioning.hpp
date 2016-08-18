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

#include "static_diagnostics.hpp"

/// @name MULTIVERSION_DECORATOR_<FEATURE> function definition decorators
///@{
#if defined(__SSE2__) || __SSE2__==1
#define MULTIVERSION_DECORATOR_SSE2		_sse2
#endif
#if defined(__SSE4_2__) && __SSE4_2__==1
#define MULTIVERSION_DECORATOR_SSE4_2	_sse4_2
#endif
#if defined(__POPCNT__) && __POPCNT__==1
#define MULTIVERSION_DECORATOR_POPCNT	_popcnt
#elif defined(__SSE4_2__) && __SSE4_2__==1
#define MULTIVERSION_DECORATOR_POPCNT	_no_popcnt
#else
#define MULTIVERSION_DECORATOR_POPCNT	/* empty if not used in conjunction with sse4.2 */
#endif
///@}

#if defined(MULTIVERSION_DECORATOR_SSE4_2)
#define MULTIVERSION(funcname) TOKEN_APPEND(TOKEN_APPEND(funcname, MULTIVERSION_DECORATOR_SSE4_2), MULTIVERSION_DECORATOR_POPCNT)
#elif defined(MULTIVERSION_DECORATOR_SSE2)
#define MULTIVERSION(funcname) TOKEN_APPEND(funcname, MULTIVERSION_DECORATOR_SSE2)
#endif

/// This macro would be what expands to the multiversion function definition under gcc.
/// So something like this in a .c/.cpp file:
///   void *memcpy(void *, const void *, size_t) __attribute__ ((ifunc ("resolve_memcpy")));
/// Note that is not a declaration, but a definition.
/// Anyway, since we can't rely on gcc's ifunc() functionality, we'll macro up something similar.
///
/// Call this macro like this from your cpp file:
///
///   MULTIVERSION_DEF(FileScanner::CountLinesSinceLastMatch, SINGLE_ARG(size_t (*FileScanner::CountLinesSinceLastMatch)(const char * __restrict__ prev_lineno_search_end,
///		const char * __restrict__ start_of_current_match) noexcept), resolve_CountLinesSinceLastMatch)
///
/// Be sure to note the SINGLE_ARG() around the second param.
///
/// @todo Now that I have this working, I think I don't like it.  It just hides what is almost exactly the
/// gcc syntax for this, and if we're not going to ever use gcc's ifunc(), probably just obfuscates things.
/// Probably won't get used unless I can come up with a good reason.
#define MULTIVERSION_DEF(funcname, func_type_def, ifunc_resolver_name) \
	/* ifunc()-like resolver for funcname(). */ \
	extern "C"	void * ifunc_resolver_name (void) ; \
	/* static initialization line for the function pointer. */ \
	func_type_def = reinterpret_cast<decltype(funcname)>(:: ifunc_resolver_name ());


#endif /* SRC_LIBEXT_MULTIVERSIONING_HPP_ */
