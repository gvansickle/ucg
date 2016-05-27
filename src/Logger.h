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

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <config.h>

#include <iostream>
#include <sstream>

/// @todo Enabled/Disabled configuration, redirecting to streams/files, timestamp, thread ID, maybe sorting by timestamp....

class Logger
{
public:
	Logger();
	virtual ~Logger()
	{
		std::cerr << m_tempstream.str();
	}

	/// We'll use this stringstream to buffer up whatever the caller wants to log, then send it
	/// to the target stream (e.g. std::cerr) in one operation.  That way we don't have to worry about
	/// concurrency issues.
	std::stringstream m_tempstream;
};

class INFO : public Logger
{
public:
	INFO() { m_tempstream << "INFO: "; };
	~INFO() override = default;

	static bool IsEnabled() noexcept { return true; };
};


#define LOG(logger) logger::IsEnabled() && logger().m_tempstream

#endif /* SRC_LOGGER_H_ */
