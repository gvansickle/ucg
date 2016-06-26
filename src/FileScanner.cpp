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

#include "config.h"

#include <immintrin.h>

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
#include <libext/string.hpp>
#include <thread>
#include <mutex>
#include <cstring> // For memchr().
#ifndef HAVE_SCHED_SETAFFINITY
#else
	#include <sched.h>
#endif

#include "ResizableArray.h"

static std::mutex f_assign_affinity_mutex;

FileScanner::CLSLM_type FileScanner::CountLinesSinceLastMatch = &FileScanner::resolve_CountLinesSinceLastMatch;


std::unique_ptr<FileScanner> FileScanner::Create(sync_queue<std::string> &in_queue,
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

FileScanner::FileScanner(sync_queue<std::string> &in_queue,
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
	std::string next_string;
	while(m_in_queue.wait_pull(std::move(next_string)) != queue_op_status::closed)
	{
		try
		{
			// Try to open and read the file.  This could throw.
			LOG(INFO) << "Attempting to scan file \'" << next_string << "\'";
			//steady_clock::time_point start = steady_clock::now();
			File f(next_string, file_data_storage);
			//steady_clock::time_point end = steady_clock::now();
			//accum_elapsed_time += (end - start);
			total_bytes_read += f.size();


			MatchList ml(next_string);


			if(f.size() == 0)
			{
				LOG(INFO) << "WARNING: Filesize of \'" << next_string << "\' is 0, skipping.";
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

__attribute__((target("sse4.2")))
size_t FileScanner::CountLinesSinceLastMatch_sse4_2(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
{
#if 1
	size_t num_lines_since_last_match = 0;

	const char * last_ptr = prev_lineno_search_end;
	size_t len = start_of_current_match-prev_lineno_search_end;
	__m128i looking_for = _mm_set1_epi8('\n');
	for(size_t i = 0; i<len; last_ptr+=16, i += 16)
	{
		int substr_len = len-i < 16 ? len-i : 16;
		__m128i substr = _mm_loadu_si128((const __m128i*)last_ptr);
		__m128i match_mask = _mm_cmpestrm(substr, substr_len, looking_for, 16, _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_BIT_MASK);
		num_lines_since_last_match += __builtin_popcountll(_mm_cvtsi128_si64(match_mask));
	}

	return num_lines_since_last_match;
#else
	return CountLinesSinceLastMatch_default(prev_lineno_search_end, start_of_current_match);
#endif
}

#if 0
extern "C"
{
static void (*resolve_CountLinesSinceLastMatch (void)) (void)
{
	return (void(*)())FileScanner::CountLinesSinceLastMatch_default;
}
}
#endif

size_t FileScanner::resolve_CountLinesSinceLastMatch(const char * __restrict__ prev_lineno_search_end,
			const char * __restrict__ start_of_current_match) //noexcept //__attribute__((ifunc("resolve_CountLinesSinceLastMatch")));
{
	/// @todo Probably needs some attention paid to multithreading.
	if(__builtin_cpu_supports("sse4.2"))
	{
		LOG(INFO) << "Using sse4.2 CountLinesSinceLastMatch";
		CountLinesSinceLastMatch = &FileScanner::CountLinesSinceLastMatch_sse4_2;
	}
	else
	{
		LOG(INFO) << "Using default CountLinesSinceLastMatch";
		CountLinesSinceLastMatch = &FileScanner::CountLinesSinceLastMatch_default;
	}

	return CountLinesSinceLastMatch(prev_lineno_search_end, start_of_current_match);
}

