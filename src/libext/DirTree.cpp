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

/** @file  */

#include <config.h>

#include "DirTree.h"

#include <fcntl.h> // For AT_FDCWD
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <iostream>
#include <queue>
#include <map>

DirTree::DirTree()
{
	// TODO Auto-generated constructor stub

}

DirTree::~DirTree()
{
	// TODO Auto-generated destructor stub
}

static void Closer(int * ptr_to_at_fd) noexcept
{
	close(*ptr_to_at_fd);
}

std::shared_ptr<int> make_shared_fd(int fd)
{
	return std::shared_ptr<int>(new int(fd), [=](int*fd){ if(*fd > 0) { std::cout << "CLOSING FD " << *fd << std::endl; close(*fd); } });
}

struct rcfd
{
	int m_ref_counted_file_desc {-1};
	size_t m_refcount {0};
};

class AtFD
{
public:
	AtFD() = default;
	AtFD(const AtFD& other)
	{
		inc_refcount(other.m_real_file_descriptor);
		m_real_file_descriptor = other.m_real_file_descriptor;
	}
	~AtFD()
	{
		if(m_real_file_descriptor == -1 || m_real_file_descriptor == AT_FDCWD)
			return;

		auto it = m_refcount_map.find(m_real_file_descriptor);
		it->second--;
		if(it->second == 0)
		{
			std::cout << "DELETE AND CLOSE: " << m_real_file_descriptor << '\n';
			m_refcount_map.erase(it);
			close(m_real_file_descriptor);
		}
	}

	int get_at_fd() const { return m_real_file_descriptor; };

	static AtFD weak_dup(int fd)
	{
		AtFD retval;

		retval.m_real_file_descriptor = fd;

		if(fd == AT_FDCWD)
		{
			return retval;
		}

		auto it = m_refcount_map.find(fd);
		if(it == m_refcount_map.end())
		{
			// Hasn't been duped yet.
			std::cout << "NEW dup, original = " << fd << '\n';
			retval.m_real_file_descriptor = dup(fd);
			m_refcount_map[retval.m_real_file_descriptor] = 1;
		}
		else
		{
			std::cout << "REFCOUNT dup, original = " << fd << '\n';
			it->second++;
			retval.m_real_file_descriptor = it->first;
		}

		return retval;
	}

	void operator=(AtFD other)
	{
		/// @todo get this to work here: std::swap(*this, other);
		inc_refcount(other.m_real_file_descriptor);
		m_real_file_descriptor = other.m_real_file_descriptor;
	};

	bool is_valid() const noexcept
	{
		if (m_real_file_descriptor > 0 || m_real_file_descriptor == AT_FDCWD)
		{
			return true;
		}
		else
		{
			return false;
		}
	};

private:
	static std::map<int, size_t> m_refcount_map;
	static void inc_refcount(int fd) { if(fd >= 0) { m_refcount_map.at(fd)++;}; };
	int m_real_file_descriptor {-1};
};

std::map<int, size_t> AtFD::m_refcount_map;

class DirStackEntry
{
public:
	DirStackEntry(AtFD parent_dir, std::string path)
	{
		at_fd = parent_dir;
		this->path = path;
	};

	std::string path;
	AtFD at_fd;
};

void DirTree::Read(std::vector<std::string> start_paths)
{
#if 1
	int num_entries {0};
	struct stat statbuf;
	DIR *d {nullptr};
	struct dirent *dp {nullptr};
	int file_fd;

	std::queue<DirStackEntry> dir_stack;
	for(auto p : start_paths)
	{
		dir_stack.push(DirStackEntry(AtFD::weak_dup(AT_FDCWD), p));
	}

	while(!dir_stack.empty())
	{
		auto dse = dir_stack.front();
		dir_stack.pop();

		std::cout << "POP: " << dse.at_fd.get_at_fd() << '\n';

		// AT_FDCWD == Start at the cwd of the process.
		file_fd = openat(dse.at_fd.get_at_fd(), dse.path.c_str(), O_RDONLY | O_NOATIME | O_NOCTTY); // Could be a file or dir.
		d = fdopendir(file_fd);
		if(d == nullptr)
		{
			// At a minimum, this wasn't a directory.
			perror("fdopendir");
		}

		AtFD dir_atfd;

		while ((dp = readdir(d)) != NULL)
		{
			bool is_dir {false};
			bool is_file {false};
			bool is_unknown {true};

#ifdef _DIRENT_HAVE_D_TYPE
			// Reject anything that isn't a directory or a regular file.
			// If it's DT_UNKNOWN, we'll have to do a stat to find out.
			is_dir = (dp->d_type == DT_DIR);
			is_file = (dp->d_type == DT_REG);
			is_unknown = (dp->d_type == DT_UNKNOWN);
			if(!is_file && !is_dir && !is_unknown)
			{
				// It's a type we don't care about.
				continue;
			}
#endif
			const char *dname = dp->d_name;
			if(dname[0] == '.' && (dname[1] == 0 || (dname[1] == '.' && dname[2] == 0)))
			{
				//std::cerr << "skipping: " << dname << '\n';
				continue;
			}

			if(is_unknown)
			{
				// Stat the filename using the at-descriptor.
				fstatat(file_fd, dname, &statbuf, AT_NO_AUTOMOUNT);
				is_dir = S_ISDIR(statbuf.st_mode);
				is_file = S_ISREG(statbuf.st_mode);
				if(is_dir || is_file)
				{
					is_unknown = false;
				}
			}

			if(is_unknown)
			{
				std::cerr << "cannot determine file type: " << dname << ", " << statbuf.st_mode << '\n';
				continue;
			}

			if(is_file)
			{
				std::cout << "File: " << dse.path + "/" + dname << '\n';
			}
			else if(is_dir)
			{
				std::cout << "Dir: " << dse.path + "/" + dname << '\n';
				//auto dirfd = openat(dse.dir_fd, dname, O_RDONLY | O_NOATIME | O_NOCTTY);

				if(!dir_atfd.is_valid())
				{
					dir_atfd = AtFD::weak_dup(file_fd);
				}

				dir_stack.push(DirStackEntry(dir_atfd, dname));
			}
		}

		closedir(d);
	}
#else
	for(auto& p : fs::recursive_directory_iterator(start_paths[0], fs::directory_options::follow_directory_symlink))
	{
		std::cout << p << "\n";
	}
#endif
}
