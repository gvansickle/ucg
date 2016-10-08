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

#include <config.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include "lorem_ipsum.hpp"

static const std::size_t f_lorem_ipsum_text_len = std::strlen(f_lorem_ipsum_text);

int main(int argc, char **argv)
{
	std::size_t max_chars_out = 0;

	int option;
	opterr = 0;
	while((option = getopt(argc, argv, "b:")) != -1)
	{
		switch (option)
		{
		case 'b':
			// Number of bytes to output.
			max_chars_out = std::atol(optarg);
			break;
		case'?':
			if(std::isprint(optopt))
			{
				std::cerr << "ERROR: Unknown option '-" << std::string(1, (char)optopt) << "'." << std::endl;
			}
			else
			{
				std::cerr << "ERROR: Unknown option charater '0x" << std::hex << optopt << "'" << std::endl;
			}
			return 1;
		default:
			abort();
		}
	}

	size_t chars_out = 0;
	while(chars_out + f_lorem_ipsum_text_len < max_chars_out)
	{
		std::cout << f_lorem_ipsum_text << std::endl;
		chars_out += f_lorem_ipsum_text_len;
	}

	std::cerr << "Number of bytes written: " << chars_out << std::endl;
	return 0;
}



