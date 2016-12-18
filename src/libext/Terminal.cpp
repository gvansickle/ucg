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

#include "Terminal.h"

#include <sys/ioctl.h>

#include "Logger.h"

Terminal::Terminal()
{
}

Terminal::~Terminal()
{
}

uint16_t Terminal::GetColumns() const
{
	struct winsize w;
	if(ioctl(0, TIOCGWINSZ, &w) != -1)
	{
		LOG(INFO) << "Terminal columns: ioctl(TIOCGWINSZ): " << w.ws_col;
		return w.ws_col;
	}

	// Else ioctl() errored out.  Try the COLUMNS env var.
	int columns = 0;
	char * colstr = getenv("COLUMNS");
	LOG(INFO) << "Terminal columns: getenv(\"COLUMNS\"): " << colstr;
	if(colstr != NULL)
	{
		columns = atoi(colstr);
	}

	if(columns == 0)
	{
		columns = 80;
		LOG(INFO) << "Terminal columns: using default of " << columns;
	}

	return columns;
}
