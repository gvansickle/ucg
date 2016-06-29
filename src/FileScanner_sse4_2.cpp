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

#include <config.h>

#include "FileScanner.h"

#include <cstdint>   // For uintptr_t.
#include <immintrin.h>

__attribute__((target("sse4.2")))
size_t FileScanner::CountLinesSinceLastMatch_sse4_2(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
{
	size_t num_lines_since_last_match = 0;

	const char * last_ptr = prev_lineno_search_end;

	// Calculate the total number of chars we need to search for '\n's in.
	size_t len = start_of_current_match-prev_lineno_search_end;

	// Check any unaligned bytes the slow way.
	while((len > 0) && (reinterpret_cast<uintptr_t>(last_ptr) & (sizeof(__m128i)-1)))
	{
		if(*last_ptr == '\n')
		{
			++num_lines_since_last_match;
		}

		++last_ptr;
		--len;
	}

	if(len == 0)
	{
		// The string didn't have >= sizeof(__m128i) chars in it.
		return num_lines_since_last_match;
	}

	__m128i looking_for = _mm_set1_epi8('\n');
	for(size_t i = 0; i<len; last_ptr+=16, i += 16)
	{
		int substr_len = len-i < 16 ? len-i : 16;
		__m128i substr = _mm_load_si128((const __m128i*)last_ptr);
		__m128i match_mask = _mm_cmpestrm(substr, substr_len, looking_for, 16, _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_BIT_MASK);
		num_lines_since_last_match += __builtin_popcountll(_mm_cvtsi128_si64(match_mask));
	}

	return num_lines_since_last_match;
}


