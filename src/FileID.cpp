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

#include <config.h>

#include "FileID.h"

#include <fts.h>

FileID::FileID(const FTSENT *ftsent): m_path(ftsent->fts_path, ftsent->fts_pathlen),
	m_unique_file_identifier(ftsent->fts_statp->st_dev, ftsent->fts_statp->st_ino),
	m_size(ftsent->fts_statp->st_size),
	m_block_size(ftsent->fts_statp->st_blksize),
	m_blocks(ftsent->fts_statp->st_blocks)
{

}

FileID::~FileID()
{
	// TODO Auto-generated destructor stub
}

