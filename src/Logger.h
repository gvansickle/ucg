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

/**
 * @file Logger.h
 *
 * A basic multithreaded logging facility.
 *
 * @todo Redirecting to streams/files, timestamp, maybe sorting by timestamp, more log severity levels: trace, debug, info, warning, error, fatal.
 */

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <config.h>

#include <cerrno>
#include <system_error>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>


/**
 * Call this from inside the thread's callable object to set its name.
 */
void set_thread_name(const std::string &name);

std::string get_thread_name();

/**
 * Base class for all the loggers.
 */
class Logger
{
public:
	Logger() = default;
	virtual ~Logger()
	{
		// Add a newline to the stringstream and flush it.
		m_tempstream << std::endl;

		// Send it to the actual output stream.
		{
		std::unique_lock<std::mutex> lock(m_cerr_mutex);
		std::cerr << m_tempstream.str();
		}
	}

	static void Init(const char * argv0)
	{
#if defined(HAVE_GNU_C_LIB_PROGRAM_INVOCATION_NAMES)
		// The GNU C Library runtime has done this work for us.
		(void)argv0;
		m_program_invocation_name = program_invocation_name;
		m_program_invocation_short_name = program_invocation_short_name;
#else
		// We're not linked to the GNU C librray (probably clang's libc).  Figure these out from argv[0].
		m_program_invocation_name = argv0;
		m_program_invocation_short_name = argv0;
		auto last_slash_pos = m_program_invocation_short_name.find_last_of("/\\");
		if(last_slash_pos == std::string::npos)
		{
			last_slash_pos = 0;
		}
		m_program_invocation_short_name = m_program_invocation_short_name.substr(last_slash_pos);
#endif
		// Set the name of the main thread.
		set_thread_name(m_program_invocation_short_name);
	}

	/// Helper function for converting a C errno into
	static std::string strerror(int c_errno = errno) noexcept
	{
		// Convert the errno to a string and return it.
		return std::error_code(c_errno, std::system_category()).message();
	}

	/// We'll use this stringstream to buffer up whatever the caller wants to log, then send it
	/// to the target stream (e.g. std::cerr) in one operation.  That way we don't have to worry about
	/// concurrency issues.
	std::stringstream m_tempstream;

protected:
	static std::string m_program_invocation_name;
	static std::string m_program_invocation_short_name;

private:
	/// Mutex for serializing writes to cerr.
	/// @note Portability: This is only needed on OSX.  Without it, individual characters written to either std::cerr or std::clog
	/// by different threads will get intermixed.  I believe this is contrary to the C++11 spec, and that each "<< whatever" (which is
	/// what we're otherwise doing here) should be atomic wrt the characters that actually get output (i.e. one flush per "<<").
	static std::mutex m_cerr_mutex;
};

/**
 * A Logger which can be enabled or disabled.
 */
template <typename T>
class EnableableLogger : public Logger
{
public:
	/// Primary constructor.
	/// @param reporting_name  This is something like "INFO" or "DEBUG", indicating the severity level.  It gets set
	///                        automatically in the INFO, DEBUG, etc. subclasses below.
	EnableableLogger(const char *reporting_name) { m_tempstream << reporting_name << ": " << get_thread_name() << ": "; };
	~EnableableLogger() noexcept override  = default;

	static void Enable(bool enable) noexcept { m_enabled = enable; };

	static bool IsEnabled() noexcept { return m_enabled; };

private:
	static bool m_enabled;
};

template <typename T>
bool EnableableLogger<T>::m_enabled { false };


/**
 * The LOG(INFO) logger.
 */
class INFO : public EnableableLogger<INFO>
{
public:
	INFO() : EnableableLogger<INFO>(__func__) { };
};


/**
 * The LOG(DEBUG) logger.
 */
class DEBUG : public EnableableLogger<DEBUG>
{
public:
	DEBUG() : EnableableLogger<DEBUG>(__func__) {};
};


/**
 * The Logger where regular warning/error messages should normally go to, via the NOTICE(), WARN(), and ERROR() macros.
 */
class STDERR : public Logger
{
public:
	STDERR() { m_tempstream << m_program_invocation_short_name << ": "; };
	~STDERR() override = default;

	static bool IsEnabled() noexcept { return true; };
};

/**
 *
 */
class STDLOG : public Logger
{
public:
	STDLOG() { m_tempstream << m_program_invocation_short_name << ": "; };
	~STDLOG() override = default;

	static bool IsEnabled() noexcept { return true; };
};


/// Helper macro for converting errno to a string, ala strerror().
#define LOG_STRERROR(...) Logger::strerror(__VA_ARGS__)

/// @name Macros for logging messages which are not intended for end-user consumption.
///@{
#define LOG(logger) logger::IsEnabled() && logger().m_tempstream << __PRETTY_FUNCTION__ << ": "
///@}

/// @name Macros for output intended for the end user.
///@{
/// @note CERR() doesn't capture the function name, unlike LOG().
#define CERR(logger) logger::IsEnabled() && logger().m_tempstream
#define NOTICE() CERR(STDERR)
#define WARN()   CERR(STDERR) << "warning: "
#define ERROR()  CERR(STDERR) << "error: "
///@}

#endif /* SRC_LOGGER_H_ */
