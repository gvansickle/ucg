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

#ifndef SRC_LIBEXT_HINTS_HPP_
#define SRC_LIBEXT_HINTS_HPP_

#include <config.h>

#include <type_traits>

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
#if !defined(assume) && !defined(HAVE_GNULIB)  // Gnulib has a comparable definition.
#if defined(HAVE___BUILTIN_UNREACHABLE)
#	define assume(exp)  if(exp) {} else { __builtin_unreachable(); }
#else
#	define assume(expr)  /* empty */
#endif
#endif

/// @name Linux-like likely() and unlikely() macros.
/// @{
/// Only define here if they haven't already been defined.  On Linux, they're defined
/// in include/linux/compiler.h.
#if !defined(likely)
#	if defined(HAVE___BUILTIN_EXPECT)
#		define likely(exp) __builtin_expect(!!(exp), true)
#	else
#		define likely(exp) /* empty */
#	endif
#endif
#if !defined(unlikely)
#	if defined(HAVE___BUILTIN_EXPECT)
#		define unlikely(exp) __builtin_expect(!!(exp), false)
#	else
#		define unlikely(exp) /* empty */
#	endif
#endif
/// @}

/// @todo Doesn't seem to have any affect on GCC 6.1.
#define assume_aligned(ptr, align)  (ptr) = static_cast<decltype(ptr)>(__builtin_assume_aligned((ptr), align))

/// @todo Come up with a way to report the STATIC_MSG_WARN()s below without having them printed on each include.

/// Check for support of the __has_cpp_attribute() macro.
/// Stub in an always-unsupported replacement if it doesn't exist.
/// Clang introduced __has_cpp_attribute() support in version 3.6.
#if !defined(__has_cpp_attribute)
	//STATIC_MSG_WARN("Compiler does not have __has_cpp_attribute() support, will not use C++11 attributes.")
#	define __has_cpp_attribute(x)  0
#endif

/// Check for support of the __has_attribute() macro.
/// GCC uses this for detection of its own __attribute__()'s.
#if !defined(__has_attribute)
	//STATIC_MSG_WARN("Compiler does not have __has_attribute() support, will not use GCC attributes.")
#	define __has_attribute(x)  0
#endif

/// Use [[maybe_unused]] after a variable's declaration to indicate that it might be unused.
#if __has_cpp_attribute(maybe_unused)
	// Have the C++17 spelling.
	//STATIC_MSG("Have C++17 [[maybe_unused]]")
#	define maybe_unused	maybe_unused
#elif __has_cpp_attribute(gnu::unused)
	// Have the GCC extension spelling.
	//STATIC_MSG("Have G++ extenstion [[gnu::unused]], using for [[maybe_unused]]")
#	define maybe_unused gnu::unused
#else
	// Not supported.
	//STATIC_MSG("C++17 attribute [[maybe_unused]] not supported")
#	define maybe_unused /* */
#endif


/// Check for GCC __attributes__() we can use.
#if __has_attribute(artificial) || defined(HAVE_FUNC_ATTRIBUTE_ARTIFICIAL)
	/// Function should appear in debug info as a unit.
#	define ATTR_ARTIFICIAL __attribute__((artificial))
#else
#	define ATTR_ARTIFICIAL
#endif
#if __has_attribute(const) || defined(HAVE_FUNC_ATTRIBUTE_CONST)
	/// Function doesn't access globals, only its args, and has no side-effects.
#	define ATTR_CONST __attribute__((const))
#else
#	define ATTR_CONST
#endif
#if __has_attribute(pure) || defined(HAVE_FUNC_ATTRIBUTE_PURE)
	/// Function may access globals in addition to its args, and has no side-effects.
#	define ATTR_PURE __attribute__((pure))
#else
#	define ATTR_PURE
#endif
#if __has_attribute(alloc_size) || defined(HAVE_FUNC_ATTRIBUTE_ALLOC_SIZE)
#	define ATTR_ALLOC_SIZE(size_of_element)  __attribute__((alloc_size(size_of_element)))
#else
#	define ATTR_ALLOC_SIZE(size_of_element)
#endif
/// @todo two-param version of alloc_size().
#if __has_attribute(alloc_align) || defined(HAVE_FUNC_ATTRIBUTE_ALLOC_ALIGN)
#define ATTR_ALLOC_ALIGN(alignment_param)  __attribute__((alloc_align(alignment_param)))
#else
#define ATTR_ALLOC_ALIGN(alignment_param)
#endif
#if __has_attribute(malloc) || defined(HAVE_FUNC_ATTRIBUTE_MALLOC)
#	define ATTR_MALLOC	__attribute__((malloc))
#else
#	define ATTR_MALLOC
#endif
#if __has_attribute(noinline) || defined(HAVE_FUNC_ATTRIBUTE_NOINLINE)
#	define ATTR_NOINLINE  __attribute__((noinline))
#else
#	define ATTR_NOINLINE
#endif

#endif /* SRC_LIBEXT_HINTS_HPP_ */
