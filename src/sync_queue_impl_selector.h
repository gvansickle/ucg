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

/**
 * @file sync_queue_impl_selector.h
 *
 * Include a sync_queue<> implementation and bring it into the global namespace.
 */

#ifndef SYNC_QUEUE_IMPL_SELECTOR_H
#define SYNC_QUEUE_IMPL_SELECTOR_H

#include <config.h>

#ifdef USE_SYNC_QUEUE_BOOST
#include <boost/thread/sync_queue.hpp>

using boost::concurrent::sync_queue;
using boost::concurrent::queue_op_status;
#else
#include "sync_queue.h"
// Our own sync_queue is already in the global namespace.
#endif

#endif // SYNC_QUEUE_IMPL_SELECTOR_H
