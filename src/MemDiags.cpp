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

#include "MemDiags.h"

#include <malloc.h>

#if 0
static void md_init_hook(void)
{
	old_malloc_hook = __malloc_hook;
	old_free_hook = __free_hook;
	__malloc_hook = md_malloc_hook;
	__free_hook = md_free_hook;
}

/* Override initializing hook from the C library. */
void (*__malloc_initialize_hook) (void) = my_init_hook;
#endif

MemDiags::MemDiags()
{
	// TODO Auto-generated constructor stub

}

void MemDiags::PrintStats() const
{
	malloc_stats();
}
