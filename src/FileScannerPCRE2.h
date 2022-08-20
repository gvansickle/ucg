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

#ifndef SRC_FILESCANNERPCRE2_H_
#define SRC_FILESCANNERPCRE2_H_

#include <config.h>

#include "FileScanner.h"

#include <libext/memory.hpp>

#if HAVE_LIBPCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#if HAVE_LIBPCRE2
/// @name Custom deleters for the PCRE2 objects we'll be using.
/// These are implemented as specializations of the std::default_delete<> template.
/// @{
namespace std
{

	template<>
	struct default_delete<pcre2_match_data>
	{
		void operator()(pcre2_match_data *ptr)
		{ pcre2_match_data_free(ptr); };
	};

	template<>
	struct default_delete<pcre2_match_context>
	{
		void operator()(pcre2_match_context *mctx)
		{ pcre2_match_context_free(mctx); };
	};

}
/// @}
#endif  // HAVE_LIBPCRE2

class FileScannerPCRE2: public FileScanner
{
public:
	FileScannerPCRE2(sync_queue<std::shared_ptr<FileID>> &in_queue,
			sync_queue<MatchList> &output_queue,
			std::string regex,
			bool ignore_case,
			bool word_regexp,
			bool pattern_is_literal);
	~FileScannerPCRE2() override;

	/**
	 * Returns the version string of the PCRE2 library.
	 * @return
	 */
	static std::string GetPCRE2Version() noexcept;

	void ThreadLocalSetup(int thread_count) final;

private:

	/**
	 * Perform post-compilation/pre-scan analysis of the regular expression to determine if there are
	 * any additional ways we can assist the PCRE2 engine.
	 */
	void AnalyzeRegex(const std::string &regex_passed_in) noexcept;

	/**
	 * Scan @a file_data for matches of m_pcre2_regex using libpcre2.  Add hits to @a ml.
	 *
	 * @param file_data
	 * @param file_size
	 * @param ml
	 */
	void ScanFile(int thread_index, const char * __restrict__ file_data, size_t file_size, MatchList &ml) final;

	static std::string PCRE2ErrorCodeToErrorString(int errorcode);

#if HAVE_LIBPCRE2
	/// The compiled libpcre2 regex.
	/// @todo Make this a unique_ptr<>, RAII-ify it.
	//std::unique_ptr<pcre2_code, void(*)(pcre2_code*)> m_pcre2_regex;
	pcre2_code *m_pcre2_regex;

	/// @note This and m_match_context are a pseudo-thread_local mechanism for systems which
	/// don't support real C++ thread_local's (older Mac OS X).
	std::vector<std::unique_ptr<pcre2_match_data>> m_match_data;

	std::vector<std::unique_ptr<pcre2_match_context>> m_match_context;

#endif

	bool m_use_first_code_unit_table { false };

	bool m_use_range_pair_table { false };
};

#endif /* SRC_FILESCANNERPCRE2_H_ */
