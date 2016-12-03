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

#include <libext/cpuidex.hpp>
#include <libext/multiversioning.hpp>

#include "Logger.h"
#include "FileScanner.h"
#include "FileScannerCpp11.h"
#include "FileScannerPCRE.h"
#include "FileScannerPCRE2.h"
#include "File.h"
#include "Match.h"
#include "MatchList.h"

#include <iostream>
#include <string>
#include <future/string.hpp>
#include <libext/string.hpp>
#include <thread>
#include <mutex>
#include <cstring> // For memchr().
#include <cstddef> // For ptrdiff_t
#include <cctype>
#ifndef HAVE_SCHED_SETAFFINITY
#else
	#include <sched.h>
#endif

#include "ResizableArray.h"

/// @todo REMOVE
#include <immintrin.h>


static std::mutex f_assign_affinity_mutex;

/// Resolver function for determining the best version of CountLinesSinceLastMatch to call.
/// Does its work at static init time, so incurs no call-time overhead.
extern "C"	void * resolve_CountLinesSinceLastMatch(void);

/// Definition of the multiversioned CountLinesSinceLastMatch function.
size_t (*FileScanner::CountLinesSinceLastMatch)(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
		= reinterpret_cast<decltype(FileScanner::CountLinesSinceLastMatch)>(::resolve_CountLinesSinceLastMatch());


std::unique_ptr<FileScanner> FileScanner::Create(sync_queue<FileID> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp,
			bool pattern_is_literal,
			RegexEngine engine)
{
	std::unique_ptr<FileScanner> retval;

	switch(engine)
	{
	case RegexEngine::CXX11:
		retval.reset(new FileScannerCpp11(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal));
		break;
	case RegexEngine::PCRE:
		retval.reset(new FileScannerPCRE(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal));
		break;
	case RegexEngine::PCRE2:
		retval.reset(new FileScannerPCRE2(in_queue, output_queue, regex, ignore_case, word_regexp, pattern_is_literal));
		break;
	default:
		// Should never get here.  Throw.
		throw FileScannerException(std::string("invalid RegexEngine specified: ") + std::to_string(static_cast<int>(engine)));
		break;
	}

	return retval;
}

FileScanner::FileScanner(sync_queue<FileID> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : m_ignore_case(ignore_case), m_word_regexp(word_regexp), m_pattern_is_literal(pattern_is_literal),
				m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex),
				m_next_core(0), m_use_mmap(false), m_manually_assign_cores(false)
{
}

FileScanner::~FileScanner()
{
}

void FileScanner::Run(int thread_index)
{
	// Set the name of the thread.
	set_thread_name("FILESCAN_" + std::to_string(thread_index));

	if(m_manually_assign_cores)
	{
		// Spread the scanner threads across cores.  Linux at least doesn't seem to want to do that by default.
		AssignToNextCore();
	}

	// Create a reusable, resizable buffer for the File() reads.
	auto file_data_storage = std::make_shared<ResizableArray<char>>();

	using namespace std::chrono;
	steady_clock::duration accum_elapsed_time {0};
	long long total_bytes_read {0};

	// Pull new filenames off the input queue until it's closed.
	FileID next_file;
	while(m_in_queue.wait_pull(std::move(next_file)) != queue_op_status::closed)
	{
		try
		{
			// Try to open and read the file.  This could throw.
			LOG(INFO) << "Attempting to scan file \'" << next_file.GetPath() << "\'";
			//steady_clock::time_point start = steady_clock::now();
			File f(next_file, file_data_storage);
			//steady_clock::time_point end = steady_clock::now();
			//accum_elapsed_time += (end - start);
			total_bytes_read += f.size();


			MatchList ml(next_file.GetPath());


			if(f.size() == 0)
			{
				LOG(INFO) << "WARNING: Filesize of \'" << next_file.GetPath() << "\' is 0, skipping.";
				continue;
			}

			const char *file_data = f.data();
			size_t file_size = f.size();

			// Scan the file data for occurrences of the regex, sending matches to the MatchList ml.
			ScanFile(file_data, file_size, ml);

			if(!ml.empty())
			{
				// Force move semantics here.
				m_output_queue.wait_push(std::move(ml));
			}
		}
		catch(const FileException &error)
		{
			// The File constructor threw an exception.
			ERROR() << error.what();
		}
		catch(const std::system_error& error)
		{
			// A system error.  Currently should only be errors from File.
			ERROR() << error.code() << " - " << error.code().message();
		}
		catch(...)
		{
			// Rethrow whatever it was.
			throw;
		}
	}

#if 0
	duration<double> elapsed = duration_cast<duration<double>>(accum_elapsed_time);
	LOG(INFO) << "Total bytes read = " << total_bytes_read << ", elapsed time = " << elapsed.count() << ", Bytes/Sec=" << total_bytes_read/elapsed.count() << std::endl;
#endif
}

void FileScanner::AssignToNextCore()
{
#ifdef HAVE_SCHED_SETAFFINITY

	// Prevent the multiple threads from stepping on each other and screwing up m_next_core.
	std::lock_guard<std::mutex> lg {f_assign_affinity_mutex};

	cpu_set_t cpuset;

	// Clear the cpu_set_t.
	CPU_ZERO(&cpuset);

	// Set the bit of the next CPU.
	CPU_SET(m_next_core, &cpuset);

	sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	// Increment so we use the next core the next time.
	m_next_core++;
	m_next_core %= std::thread::hardware_concurrency();
#endif
}

//__attribute__((target("default")))
size_t FileScanner::CountLinesSinceLastMatch_default(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
{
	size_t num_lines_since_last_match = 0;

	const char * last_ptr = prev_lineno_search_end;
	while(1)
	{
		last_ptr = (const char*)std::memchr((const void*)last_ptr, '\n', start_of_current_match-last_ptr);
		if(last_ptr != NULL)
		{
			++num_lines_since_last_match;
			++last_ptr;
		}
		else
		{
			break;
		}
	}

	return num_lines_since_last_match;
}

const char * FileScanner::LiteralPrescan(std::string regex, const char * __restrict__ start_of_array, const char * __restrict__ end_of_array) noexcept
{
	size_t prefix_literal_len { 0 };
	bool at_least_one_upper = false;
	for(auto i : regex)
	{
		if(std::isalnum(i))
		{
			prefix_literal_len++;
			if(std::isupper(i))
			{
				at_least_one_upper=true;
			}
		}
	}

	if((prefix_literal_len == 0) || (!at_least_one_upper))
	{
		return start_of_array;
	}

	// Search for the literal prefix.
	const char * __restrict__ possible_match_end { start_of_array+(prefix_literal_len-1) };
	const char * __restrict__ possible_match_start { start_of_array };
	do
	{
		possible_match_end = (const char*)std::memchr(possible_match_end, regex[prefix_literal_len-1], (end_of_array-possible_match_end));
		if(possible_match_end != NULL)
		{
			// Check if the string matched.
			possible_match_start = possible_match_end - (prefix_literal_len-1);
			if(std::memcmp(possible_match_start, regex.c_str(), prefix_literal_len-1) == 0)
			{
				// Found the prefix string.
				return possible_match_start;
			}
			else
			{
				possible_match_end+=1;
			}
		}
	} while(possible_match_end != nullptr);

	// If we drop out of the while(), we didn't find it.
	return start_of_array;
}

extern "C" void * resolve_CountLinesSinceLastMatch(void)
{
	void *retval;

	if(sys_has_sse4_2() && sys_has_popcnt())
	{
		retval = reinterpret_cast<void*>(&FileScanner::CountLinesSinceLastMatch_sse4_2_popcnt);
	}
	else if(sys_has_sse4_2() && !sys_has_popcnt())
	{
		retval = reinterpret_cast<void*>(&FileScanner::CountLinesSinceLastMatch_sse4_2_no_popcnt);
	}
	else if(sys_has_sse2())
	{
		retval = reinterpret_cast<void*>(&FileScanner::CountLinesSinceLastMatch_sse2);
	}
	else
	{
		retval = reinterpret_cast<void*>(&FileScanner::CountLinesSinceLastMatch_default);
	}

	return retval;
}

std::tuple<const char *, size_t> FileScanner::GetEOL(const char *search_start, const char * buff_one_past_end)
{
	std::ptrdiff_t max_len = buff_one_past_end-search_start;
	const char * p = (const char *)std::memchr(search_start, '\n', max_len);

	if(p == nullptr)
	{
		// Not found.
		return std::make_tuple(buff_one_past_end, max_len);
	}
	else
	{
		return std::make_tuple(p, p-search_start);
	}
}



bool FileScanner::ConstructCodeUnitTable_default(const uint8_t *pcre2_bitmap) noexcept
{
	uint16_t out_index = 0;
	for(uint16_t i=0; i<256; ++i)
	{
		if((pcre2_bitmap[i/8] & (0x01 << (i%8))) == 0)
		{
			// This bit isn't set, skip to the next one.
			continue;
		}
		else
		{
			/// @todo This depends on little-endianness.
			m_compiled_cu_bitmap[out_index] = i;
			out_index++;
		}
	}
	m_end_index = out_index;
	return true;
}


const char * FileScanner::FindFirstPossibleCodeUnit_default(const char * __restrict__ cbegin, size_t len) noexcept
{
	const char *first_possible_cu = nullptr;
	if(m_end_index > 1)
	{
#if 0
		first_possible_cu = std::find_first_of(cbegin, cbegin+len, m_compiled_cu_bitmap, m_compiled_cu_bitmap+m_end_index);
#else
		first_possible_cu = find_first_of_sse4_2_popcnt(cbegin, len);
#endif
	}
	else if(m_end_index == 1)
	{
#if 0
		first_possible_cu = std::find(cbegin, cbegin+len, m_compiled_cu_bitmap[0]);
		/// @note Tried memchr() here, no real difference.
#else
		// The character we're looking for, broadcast to all 16 bytes of the looking_for xmm register.
		// SSE2.
		const __m128i looking_for = _mm_set1_epi8(m_compiled_cu_bitmap[0]);
		for(size_t i=0; i<len; i+=32)
		{
			// Load an xmm register with 16 aligned bytes.  SSE2, L/Th: 1/0.25-0.5, plus cache effects.
			__m128i xmm1 = _mm_loadu_si128((const __m128i *)(cbegin+i));
			// Compare the 16 bytes with searchchar.  SSE2, L/Th: 1/0.5.
			// match_bytemask will contain a 0xFF for a matching byte, 0 for a non-matching byte.
			__m128i match_bytemask = _mm_cmpeq_epi8(xmm1, looking_for);
			// Convert the bytemask into a bitmask in the lower 16 bits of match_bitmask.  SSE2, L/Th: 3-1/1
			uint32_t match_bitmask = _mm_movemask_epi8(match_bytemask);

			__m128i xmm2 = _mm_loadu_si128((const __m128i *)(cbegin+i+16));
			__m128i xmm3 = _mm_cmpeq_epi8(xmm2, looking_for);
			match_bitmask |= _mm_movemask_epi8(xmm3) << 16;

			// The above should never result in more than the bottom 16 bits of match_bitmask being set.
			// Hint this to the compiler.  This prevents gcc from adding an unnecessary movzwl %rNw,%rNd before the popcount16() call.
			//assume(match_bitmask <= 0xFFFFU);

			// Did we find any chars?
			if(match_bitmask > 0)
			{
				// Find the first bit set.
				auto lowest_bit = findfirstsetbit(match_bitmask);
				if(lowest_bit > 0 && (i + lowest_bit -1 < len))
				{
					return cbegin + i + lowest_bit - 1;
				}
			}
		}
		return cbegin+len;
#endif
	}
	else
	{
		first_possible_cu = cbegin+len;
	}
	return first_possible_cu;
}

