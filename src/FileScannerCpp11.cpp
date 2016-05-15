/*
 * Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "FileScannerCpp11.h"

FileScannerCpp11::FileScannerCpp11(sync_queue<std::string> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : FileScanner(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal)
{
	// TODO Auto-generated constructor stub

}

FileScannerCpp11::~FileScannerCpp11()
{
	// TODO Auto-generated destructor stub
}

void FileScannerCpp11::ScanFile(const char * __restrict__ file_data, size_t file_size, MatchList &ml)
{

}
