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

/** @file  */

#ifndef SRC_LIBEXT_EXCEPTION_HPP_
#define SRC_LIBEXT_EXCEPTION_HPP_

#include <config.h>

#include <future/string.hpp>
#include <exception>

inline void print_exception_stack(const std::exception& e, size_t indentation_level = 0)
{
	std::cerr << std::string(indentation_level, '\t') << "Exception: " << e.what() << "\n";

	try
	{
		std::rethrow_if_nested(e);
	}
	catch (const std::exception& e)
	{
		print_exception_stack(e, indentation_level+1);
	}
	catch(...)
	{

	}
}

#define RETHROW(str) std::throw_with_nested(std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":" + std::to_string(__LINE__) + ": " + (str) ))

#endif /* SRC_LIBEXT_EXCEPTION_HPP_ */
