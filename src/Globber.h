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

#ifndef GLOBBER_H_
#define GLOBBER_H_


#include <string>
#include <thread>
#include <unordered_set>
#include <boost/thread/sync_queue.hpp>

/*
 *
 */
class Globber
{
public:
	Globber(std::string start_dir, boost::concurrent::sync_queue<std::string> &out_queue);
	virtual ~Globber();

	void Run();

private:
	std::string m_start_dir;
	boost::concurrent::sync_queue<std::string>& m_out_queue;

	/// File extensions which will be examined.
	std::unordered_set<std::string> m_include_extensions;
};

#endif /* GLOBBER_H_ */
