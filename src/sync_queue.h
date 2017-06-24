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

/** @file Simple unbounded synchronized queue class. */

#ifndef SYNC_QUEUE_H_
#define SYNC_QUEUE_H_

#include <config.h>

#include <mutex>
#include <condition_variable>
#include <queue>
#include <libext/hints.hpp>

#if TODO
#include <scoped_allocator>
#include <ext/mt_allocator.h>
#endif

enum class queue_op_status
{
	success,
	empty,
	full,
	closed,
	busy,
	timeout,
	not_ready
};


/**
 * Simple unbounded synchronized queue class.
 *
 * The interface implemented here is loosely based on ISO/IEC JTC1 SC22 WG21 N3533
 * <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3533.html> and subsequent work noted here:
 * <http://www.boost.org/doc/libs/1_63_0/doc/html/thread/compliance.html#thread.compliance.cxx1y.queue>.
 * The wait_for_worker_completion() interface is my own addition.
 */
template <typename ValueType>
class sync_queue
{
#ifdef TODO
	using mt_deque = std::deque<ValueType, std::scoped_allocator_adaptor<__gnu_cxx::__mt_alloc<ValueType>>>;
#else
	using mt_deque = std::deque<ValueType>;
#endif

	//std::queue<ValueType, mt_deque> m_underlying_queue;
	mt_deque m_underlying_queue;

public:

	using size_type = typename mt_deque::size_type;

	sync_queue() {};
	~sync_queue() {};

	size_type size() const noexcept __attribute__((noinline))
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_underlying_queue.size();
	}

	void close()
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_closed = true;

		// Unlock the mutex immediately prior to notify.  This prevents a waiting thread from being immediately woken up
		// by the notify, and then blocking because we still hold the mutex.
		lock.unlock();

		// Notify all threads waiting on the queue's condition variable that it's just been closed.
		m_cv.notify_all();
	}

	queue_op_status push_back(const ValueType& x) __attribute__((noinline))
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// Is the queue closed?
		if(m_closed)
		{
			// Yes, fail the push.
			return queue_op_status::closed;
		}

		// Push via copy.
		m_underlying_queue.push_back(x);

		// Unlock the mutex immediately prior to notify.  This prevents a waiting thread from being immediately woken up
		// by the notify, and then blocking because we still hold the mutex.
		lock.unlock();

		// Notify one thread waiting on the queue's condition variable that it now has something to pull.
		// Note that since we only pushed one item, we only need to notify one waiting thread.  This prevents waking up
		// all waiting threads, all but one of which will end up immediately blocking again.
		m_cv.notify_one();

		return queue_op_status::success;
	}

	queue_op_status push_back(ValueType&& x) __attribute__((noinline))
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// Is the queue closed?
		if(m_closed)
		{
			// Yes, fail the push.
			return queue_op_status::closed;
		}

		// Push via move.
		m_underlying_queue.push_back(std::move(x));

		// Unlock the mutex immediately prior to notify.  This prevents a waiting thread from being immediately woken up
		// by the notify, and then blocking because we still hold the mutex.
		lock.unlock();

		// Notify one thread waiting on the queue's condition variable that it now has something to pull.
		// Note that since we only pushed one item, we only need to notify one waiting thread.  This prevents waking up
		// all waiting threads, all but one of which will end up immediately blocking again.
		m_cv.notify_one();

		return queue_op_status::success;
	}

	/**
	 * Push multiple values from a container onto the queue in one operation.  Moves the elements.
	 *
	 * @param ContainerOfValues
	 * @return
	 */
	template <typename T, typename Unused = typename T::value_type>
	queue_op_status __attribute__((noinline)) push_back(T& ContainerOfValues)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// Is the queue closed?
		if(m_closed)
		{
			// Yes, fail the push.
			return queue_op_status::closed;
		}

		// Push via move.
		m_underlying_queue.insert(m_underlying_queue.end(), /// @note This should be cend() AFAICT, but that won't compile on old clang.
				std::make_move_iterator(ContainerOfValues.begin()),
				std::make_move_iterator(ContainerOfValues.end()));

		// Unlock the mutex immediately prior to notify.  This prevents a waiting thread from being immediately woken up
		// by the notify, and then blocking because we still hold the mutex.
		lock.unlock();

		// Notify any threads waiting on the queue's condition variable that they now have something to pull.
		m_cv.notify_all();

		return queue_op_status::success;
	}

	queue_op_status pull_front(ValueType& x) __attribute__((noinline))
	{
		// Using a unique_lock<> here vs. a lock_guard<> because we'll be using a condition variable, which needs
		// to unlock the mutex.
		std::unique_lock<std::mutex> lock(m_mutex);

		m_num_waiting_threads++;

		if(m_num_waiting_threads == m_num_waiting_threads_notification_level)
		{
			m_cv_complete.notify_all();
		}

		// Wait until the queue is not empty, or somebody closes the sync_queue<>.
		m_cv.wait(lock, [this](){ return !m_underlying_queue.empty() || m_closed; });

		m_num_waiting_threads--;

		// Check if we've be awoken to a closed and empty queue.
		if(m_underlying_queue.empty() && m_closed)
		{
			// We have, let the caller know.
			return queue_op_status::closed;
		}

		// Otherwise, we have something in the queue to pull off.
		x = m_underlying_queue.front();
		m_underlying_queue.pop_front();

		return queue_op_status::success;
	}

	queue_op_status pull_front(ValueType&& x) __attribute__((noinline))
	{
		// Using a unique_lock<> here vs. a lock_guard<> because we'll be using a condition variable, which needs
		// to unlock the mutex.
		std::unique_lock<std::mutex> lock(m_mutex);

		m_num_waiting_threads++;

		if(m_num_waiting_threads == m_num_waiting_threads_notification_level)
		{
			m_cv_complete.notify_all();
		}

		// Wait until the queue is not empty, or somebody closes the sync_queue<>.
		m_cv.wait(lock, [this](){ return !m_underlying_queue.empty() || m_closed; });

		m_num_waiting_threads--;

		// Check if we've be awoken to a closed and empty queue.
		if(m_underlying_queue.empty() && m_closed)
		{
			// We have, let the caller know.
			return queue_op_status::closed;
		}

		// Otherwise, we have something in the queue to pull off.
		// Use move-assignment vs. copy-assignment for efficiency.
		// Note that C++11 std::queue<>::front() returns a non-const reference as well as a const one, so
		// std::move() will work here.
		x = std::move(m_underlying_queue.front());
		m_underlying_queue.pop_front();

		return queue_op_status::success;
	}

	/**
	 *  Blocks the calling thread until:
	 *	 - The queue is empty, and
	 *	 - There are @p num_workers threads waiting to be notified of new work arriving in the queue.
	 *	 - Or, the queue is closed.
	 *
	 *  The use case here is a situation where you have one "master" thread spawning one or more worker threads which then
	 *  feed their own work queue until they're done.  The problem is, the workers won't know when they're done; they'll all
	 *  pend on wait_pull() for more work, which will never come.  To solve this, the master thread waits via this API,
	 *  and when all the workers are waiting and there's no work in the queue, the master closes the queue, which causes the
	 *  worker threads to exit, which are then joined by the master thread.
	 *
	 *	@note This is definitely not a Boost API.
	 */
	queue_op_status wait_for_worker_completion(size_t num_workers)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// If creating thread already says he set the m_num_waiting_threads_notification_level, skip this.
		if(num_workers > 0)
		{
			m_num_waiting_threads_notification_level = num_workers;
		}

		// Wake up any waiting threads, so they can check the new notification level.
		m_cv_complete.notify_all();

		m_cv_complete.wait(lock, [this](){
			return ((m_num_waiting_threads == m_num_waiting_threads_notification_level) && m_underlying_queue.empty()) || m_closed;
		});

		if(m_closed)
		{
			return queue_op_status::closed;
		}
		else
		{
			return queue_op_status::success;
		}
	}

private:

	mutable std::mutex m_mutex;

	std::condition_variable m_cv;

	std::condition_variable m_cv_complete;

	size_t m_num_waiting_threads_notification_level { 500 };

	size_t m_num_waiting_threads { 0 };

	bool m_closed { false };
};

#endif /* SYNC_QUEUE_H_ */
