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

/** @file Macros for compiler optimization hints.  */

#include <config.h>

#include <type_traits>

#ifndef SRC_LIBEXT_HINTS_HPP_
#define SRC_LIBEXT_HINTS_HPP_

#include "static_diagnostics.hpp"

/// Hint to the compiler that #exp is unconditionally true.
///
/// GCC and Clang don't have an explicit intrinsic for this, but __builtin_unreachable() coupled with
/// an if(expr) appears to both kill the control flow during compilation, resulting in no code being generated for
/// this construct, and it gives the compilers the info they need to optimize better.
///
/// The ARM tools do have an explicit __promise(expr) intrinsic which more explicitly does this same thing.
/// See http://infocenter.arm.com/help/topic/com.arm.doc.dui0472m/chr1359124207681.html.
/// @note The ARM tools apparently define __promise() in <assert.h>, though it's documented as an intrinsic.
///
#define assume(exp)  if(exp) {} else { __builtin_unreachable(); }

/// @name Linux-like likely() and unlikely() macros.
/// @{
/// Only define here if they haven't already been defined.  On Linux, they're defined
/// in include/linux/compiler.h.
#if !defined(likely)
#define likely(exp) __builtin_expect(!!(exp), true)
#endif
#if !defined(unlikely)
#define unlikely(exp) __builtin_expect(!!(exp), false)
#endif
/// @}

#define assume_aligned(ptr, align)  (ptr) = static_cast<decltype(ptr)>(__builtin_assume_aligned((ptr), align))

/// Check for support of the __has_cpp_attribute() macro.
/// Stub in an always-unsupported replacement if it doesn't exist.
#ifndef __has_cpp_attribute
	STATIC_MSG_WARN("Compiler does not have __has_cpp_attribute() support, will not use attributes.")
#	define __has_cpp_attribute(x)  0
#endif

/// Use [[maybe_unused]] after a variable's declaration to indicate that it might be unused.
#if __has_cpp_attribute(maybe_unused)
	// Have the C++17 spelling.
#	pragma message "Have C++17 [[maybe_unused]]"
#	define maybe_unused	maybe_unused
#elif __has_cpp_attribute(gnu::unused)
	// Have the GCC extension spelling.
	STATIC_MSG("Have G++ extenstion [[gnu::unused]]")
#	define maybe_unused gnu::unused
#else
	// Not supported.
#	pragma message "C++17 attribute [[maybe_unused]] not supported"
#	define maybe_unused
#endif

#endif /* SRC_LIBEXT_HINTS_HPP_ */
