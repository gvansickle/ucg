/*
 * Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef FILESCANNER_H_
#define FILESCANNER_H_

#include "config.h"

#include <stdexcept>
#include <string>
#include <memory>

#include "sync_queue_impl_selector.h"
#include "MatchList.h"


/// The regular expression engines we support.
/// @note Which of these is supported depends on which libraries were available at compile-time.
enum class RegexEngine
{
	NONE,	//!< No engine available.
	CXX11, 	//!< C++11's built-in <regex> support.
	PCRE,	//!< The original libpcre.
	PCRE2,	//!< libpcre2

#if HAVE_LIBPCRE2
	DEFAULT = PCRE2,
#elif HAVE_LIBPCRE
	DEFAULT = PCRE,
#else
	DEFAULT = NONE,
#endif
};


/**
 * FileScanner will throw this if it runs into trouble compiling the regex.
 */
struct FileScannerException : public std::runtime_error
{
	FileScannerException(const std::string &message) : std::runtime_error(message) {};
};


/**
 * Base class for the classes which do the actual regex scanning of the file contents.
 */
class FileScanner
{
public:

	/**
	 * Factory Method for creating a new FileScanner-derived class.
	 *
	 * @param in_queue
	 * @param output_queue
	 * @param regex
	 * @param ignore_case
	 * @param word_regexp
	 * @param pattern_is_literal
	 * @param engine
	 * @return
	 */
	static std::unique_ptr<FileScanner> Create(sync_queue<std::string> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp,
			bool pattern_is_literal,
			RegexEngine engine = RegexEngine::DEFAULT);

public:
	FileScanner(sync_queue<std::string> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp,
			bool pattern_is_literal);
	virtual ~FileScanner();

	void Run(int thread_index);

protected:

	__attribute__((target("sse4.2")))
	static size_t CountLinesSinceLastMatch(const char * __restrict__ prev_lineno_search_end,
			const char * __restrict__ start_of_current_match) noexcept;

	__attribute__((target("default")))
	static size_t CountLinesSinceLastMatch(const char * __restrict__ prev_lineno_search_end,
			const char * __restrict__ start_of_current_match) noexcept;

	bool m_ignore_case;

	bool m_word_regexp;

	bool m_pattern_is_literal;

private:

	/**
	 * Helper to assign each thread which starts Run() to a different core.
	 * Note that this currently only works on Linux, and does not appear to make
	 * a measurable difference in performance, likely because we're I/O bound regardless.
	 *
	 * Maintaining this for experimental purposes.
	 */
	void AssignToNextCore();

	/**
	 * Scan @a file_data for matches of the regex.  Add hits to @a ml.
	 *
	 * @param file_data
	 * @param file_size
	 * @param ml
	 */
	virtual void ScanFile(const char * __restrict__ file_data, size_t file_size, MatchList &ml) = 0;

	sync_queue<std::string>& m_in_queue;

	sync_queue<MatchList> &m_output_queue;

	std::string m_regex;

	int m_next_core;

	bool m_use_mmap;

	/**
	 * Switch to make Run() assign its std::thread to different cores on the machine.
	 * If false, the underlying std::thread logic is allowed to decide which threads run on
	 * which cores.  See note above for AssignToNextCore(); turning this on makes no real difference.
	 *
	 * Maintaining this for experimental purposes.
	 */
	bool m_manually_assign_cores;
};

#endif /* FILESCANNER_H_ */
