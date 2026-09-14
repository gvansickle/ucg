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

#include <config.h>

#include "OutputContext.h"

OutputContext::OutputContext(bool enable_color, bool prefix_file, bool print_line_number,
                             bool print_column, bool nullsep)
  : m_enable_color(enable_color), m_prefix_file(prefix_file),
    m_print_line_number(print_line_number),
    m_print_column(print_column), m_nullsep(nullsep)
{
	if(m_enable_color)
	{
		// Set up the active colors to be the defaults.
		m_color_filename = m_default_color_filename;
		m_color_match = m_default_color_match;
		m_color_lineno = m_default_color_lineno;
		m_color_default = m_default_color_default;
	}
}

OutputContext::~OutputContext()
{
}

