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
#include <libext/Logger.h>
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


static std::mutex f_assign_affinity_mutex;

/// Resolver function for determining the best version of CountLinesSinceLastMatch to call.
/// Does its work at static init time, so incurs no call-time overhead.
extern "C"	void * resolve_CountLinesSinceLastMatch(void);

/// Definition of the multiversioned CountLinesSinceLastMatch function.
size_t (*FileScanner::CountLinesSinceLastMatch)(const char * __restrict__ prev_lineno_search_end,
		const char * __restrict__ start_of_current_match) noexcept
		= reinterpret_cast<decltype(FileScanner::CountLinesSinceLastMatch)>(::resolve_CountLinesSinceLastMatch());


std::unique_ptr<FileScanner> FileScanner::Create(sync_queue<std::shared_ptr<FileID>> &in_queue,
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

FileScanner::FileScanner(sync_queue<std::shared_ptr<FileID>> &in_queue,
		sync_queue<MatchList> &output_queue,
		std::string regex,
		bool ignore_case,
		bool word_regexp,
		bool pattern_is_literal) : m_ignore_case(ignore_case), m_word_regexp(word_regexp), m_pattern_is_literal(pattern_is_literal),
				m_in_queue(in_queue), m_output_queue(output_queue), m_regex(regex),
				m_next_core(0), m_use_mmap(false), m_manually_assign_cores(false)
{
	LiteralMatch = resolve_LiteralMatch(this);
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
	std::shared_ptr<FileID> next_file;
	MatchList ml;
	while(m_in_queue.pull_front(std::move(next_file)) != queue_op_status::closed)
	{
		try
		{
			// Try to open and read the file.  This could throw.
			LOG(INFO) << "Attempting to scan file \'" << next_file->GetPath() << "\', fd=" << next_file->GetFileDescriptor().GetFD();

			steady_clock::time_point start = steady_clock::now();

			File f(next_file, file_data_storage);

			steady_clock::time_point end = steady_clock::now();
			accum_elapsed_time += (end - start);

			auto bytes_read = f.size();
			total_bytes_read += bytes_read;
			LOG(INFO) << "Num/total bytes read: " << bytes_read << " / " << total_bytes_read;

			if(f.size() == 0)
			{
				LOG(INFO) << "WARNING: Filesize of \'" << f.name() << "\' is 0, skipping.";
				continue;
			}

			const char *file_data = f.data();
			size_t file_size = f.size();

			// Scan the file data for occurrences of the regex, sending matches to the MatchList ml.
			ScanFile(file_data, file_size, ml);

			if(!ml.empty())
			{
				ml.SetFilename(next_file->GetPath());
				// Force move semantics here.
				m_output_queue.push_back(std::move(ml));
				ml.clear();
			}
		}
		catch(const FileException &error)
		{
			// The File constructor threw an exception.
			ERROR() << error.what();
			LOG(DEBUG) << "Caught FileException: " << error.what();
		}
		catch(const std::system_error& error)
		{
			// A system error.  Currently should only be errors from File.
			ERROR() << error.code() << " - " << error.code().message();
			LOG(DEBUG) << "Caught std::system_error: " << error.code() << " - " << error.code().message();
		}
		catch(...)
		{
			// Rethrow whatever it was.
			throw;
		}
	}

	duration<double> elapsed = duration_cast<duration<double>>(accum_elapsed_time);
	LOG(INFO) << "Total bytes read = " << total_bytes_read << ", elapsed time = " << elapsed.count() << ", Bytes/Sec=" << total_bytes_read/elapsed.count() << std::endl;
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

bool FileScanner::IsPatternLiteral(const std::string &regex) const noexcept
{
	// Search the string for any of the PCRE2 metacharacters.  This will cause some false negatives (e.g. anything with escapes
	// will be determined to be a non-literal), but is quick and easy.
	auto metachar_pos = regex.find_first_of("\\^$.[]()?*+{}|");

	bool is_lit = (metachar_pos == std::string::npos);

	return is_lit;
}

uint8_t FileScanner::GetLiteralPrefixLen(const std::string &regex) noexcept
{
	// Bail if there are any alternates anywhere in the pattern.  This avoids having to
	// deal with situations like '(cat|cab|car|cot)', which is a likely an overall loss anyway.
	auto alt_pos = regex.find_first_of("|");
	if(alt_pos != std::string::npos)
	{
		return 1;
	}

	// Otherwise, keep going until we find something non-literal.
	auto first_metachar_pos = regex.find_first_of("\\^$.[]()?*+{}");
	if(first_metachar_pos == std::string::npos)
	{
		// The whole regex was literal, we shouldn't have gotten here.
		LOG(INFO) << "No non-literal chars in regex.";
		return 1;
	}

	if(first_metachar_pos > 1)
	{
		// There is at least one possible additional literal char.
		// "Possible" because the non-literal following it could "de-literalize" it; e.g.,
		// 'abc*' has a literal prefix of only 'ab', not 'abc'.  We adjust for such situations here.
		/// @note '{' here because it could be e.g. 'abc{0,1}'.  We could get fancier about this case.
		if(std::string("?*{").find(regex[first_metachar_pos]) != std::string::npos)
		{
			// It was one of the optional modifiers.  Remove the last char.
			--first_metachar_pos;
		}
	}

	return std::min(first_metachar_pos, static_cast<decltype(first_metachar_pos)>(255));
}


int FileScanner::LiteralMatch_default(const char *file_data, size_t file_size, size_t start_offset, size_t *ovector) const noexcept
{
	int rc = 0;

	const char* str_match = (const char*)memmem((const void*)(file_data+start_offset), file_size - start_offset,
						(const void *)m_literal_search_string.get(), m_literal_search_string_len);

	if(str_match == nullptr)
	{
		// No match.
		rc = -1; //PCRE2_ERROR_NOMATCH;  /// @todo This will probably break non-PCRE2 builds.
		ovector[0] = file_size;
		ovector[1] = file_size;
	}
	else
	{
		// Found a match.
		rc = 1;
		ovector[0] = str_match - file_data;
		ovector[1] = ovector[0] + m_literal_search_string_len;
	}

	return rc;
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

FileScanner::LiteralMatch_type FileScanner::resolve_LiteralMatch(FileScanner * obj [[maybe_unused]]) noexcept
{
	FileScanner::LiteralMatch_type retval;

	if(sys_has_sse4_2())
	{
		retval = &FileScanner::LiteralMatch_sse4_2;
	}
	else
	{
		retval = &FileScanner::LiteralMatch_default;
	}

	return retval;
}

bool FileScanner::ConstructCodeUnitTable(const uint8_t *pcre2_bitmap) noexcept
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
	m_end_fpcu_table = out_index;
	return true;
}

void FileScanner::ConstructRangePairTable() noexcept
{
	// Vars for pair finding.
	uint16_t out_pair_index = 0;
	uint8_t first_range_char = 0, last_range_char = 0;

	if(m_end_fpcu_table == 0)
	{
		// No code unit table, can't be a ranges table.
		LOG(DEBUG) << "No first-possible code unit table, skipping creation of range table.";
		m_end_ranges_table = 0;
		return;
	}

	first_range_char = m_compiled_cu_bitmap[0];
	last_range_char = first_range_char;

	for(uint16_t i=1; i<m_end_fpcu_table; ++i)
	{
		// We're looking for the end of this range.
		if(m_compiled_cu_bitmap[i] == last_range_char + 1)
		{
			// We're still in the range.
			++last_range_char;
		}
		else
		{
			// Range has ended.
			m_compiled_range_bitmap[out_pair_index] = first_range_char;
			++out_pair_index;
			m_compiled_range_bitmap[out_pair_index] = last_range_char;
			++out_pair_index;

			LOG(DEBUG) << "Found range pair: [" << first_range_char << ", " << last_range_char << "]";

			// Set up for the next range.
			first_range_char = m_compiled_cu_bitmap[i];
			last_range_char = first_range_char;
		}
	}

	// Terminate the last pair.
	m_compiled_range_bitmap[out_pair_index] = first_range_char;
	++out_pair_index;
	m_compiled_range_bitmap[out_pair_index] = last_range_char;
	++out_pair_index;
	LOG(DEBUG) << "Found range pair: [" << first_range_char << ", " << last_range_char << "]";

	m_end_ranges_table = out_pair_index;
}


const char * FileScanner::FindFirstPossibleCodeUnit_default(const char * __restrict__ cbegin, size_t len) const noexcept
{
	const char *first_possible_cu = nullptr;
	if(m_end_fpcu_table > 1)
	{
#if 0
		first_possible_cu = std::find_first_of(cbegin, cbegin+len, m_compiled_cu_bitmap, m_compiled_cu_bitmap+m_end_fpcu_table);
#else
		first_possible_cu = find_first_of_sse4_2_popcnt(cbegin, len);
#endif
	}
	else if(m_end_fpcu_table == 1)
	{
#if 0
		first_possible_cu = std::find(cbegin, cbegin+len, m_compiled_cu_bitmap[0]);
		/// @note Tried memchr() here, no real difference.
#else
		first_possible_cu = find_sse4_2_popcnt(cbegin, len);
#endif
	}
	else
	{
		first_possible_cu = cbegin+len;
	}
	return first_possible_cu;
}

