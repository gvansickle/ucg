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

#ifndef SRC_OUTPUTCONTEXT_H_
#define SRC_OUTPUTCONTEXT_H_

#include <string>

/**
 * A class for encapsulating the output "context", e.g. what colors to use, whether to print the column number, etc.
 */
class OutputContext
{
public:
	OutputContext(bool output_is_tty, bool enable_color, bool print_column);
	~OutputContext();

	inline bool is_output_tty() const noexcept { return m_output_is_tty; };
	inline bool is_color_enabled() const noexcept { return m_enable_color; };
	inline bool is_column_print_enabled() const noexcept { return m_print_column; };

	/// @name Active colors.
	/// @{
	std::string m_color_filename;
	std::string m_color_match;
	std::string m_color_lineno;
	std::string m_color_default;
	/// @}

private:

	bool m_output_is_tty;

	/// Whether to output color or not.  Determined by logic in OutputTask's constructor.
	bool m_enable_color;

	/// Whether to print the column number of the first match or not.
	bool m_print_column;

	/// @name Default output colors.
	/// @{
	// ANSI SGR parameter setting sequences for setting the color and boldness of the output text.
	// SGR = Select Graphic Rendition.
	// A note on the "\x1B[K" at the end of each sequence:
	// This is the "Erase in Line" sequence, which in this form clears the terminal from the cursor position
	// to the end of the line.  This is needed after every SGR color sequence to prevent scrolling at the bottom of
	// the terminal at the wrong time from causing that entire line to have the non-default background color.  It also
	// prevents issues with Horizontal Tab not setting the background to the current color.
	// A more extensive discussion of this topic is available in the GNU grep source (git://git.savannah.gnu.org/grep.git,
	// see src/grep.c), which is where I got this solution from.
	std::string m_default_color_filename {"\x1B[32;1m\x1B[K"}; // 32=green, 1=bold
	std::string m_default_color_match {"\x1B[30;43;1m\x1B[K"}; // 30=black, 43=yellow bkgnd, 1=bold
	std::string m_default_color_lineno {"\x1B[33;1m\x1B[K"};   // 33=yellow, 1=bold
	std::string m_default_color_default {"\x1B[0m\x1B[K"};     // Reset/normal (all attributes off).
	/// @}
};

#endif /* SRC_OUTPUTCONTEXT_H_ */
