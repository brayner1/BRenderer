#include "ThreadPool.h"

#include <Core/Threading/Work.h>

#include <iostream>

namespace brr::thread
{
    ThreadPool& ThreadPool::GetDefaultPool()
    {
		static ThreadPool thread_pool {};
		return thread_pool;
    }

	ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency() - 1)
	{

	}

	ThreadPool::ThreadPool(size_t num_threads) : m_workQueue(), m_workerThreads(), m_runningThreads(0)
    {
		std::cout << "Creating ThreadPool with " << num_threads << "threads\n";
		// Pre-allocate the pool
		m_workerThreads.reserve(num_threads);

		for (int i = 0; i < num_threads; ++i)
		{
			// Create worker threads
			m_workerThreads.emplace_back(&ThreadPool::WorkerThreadFunc, this, i + 1);
		}
	}

	ThreadPool::~ThreadPool()
	{
		// Indicate that worker threads should stop
		m_stopWorkerThreads = true;
		// Notify sleeping worker threads
		m_workReadyCondition.notify_all();
		// Join all the threads with the main thread.
		for (std::thread& thread : m_workerThreads)
		{
			if (thread.joinable())
				thread.join();
		}
	}

	void ThreadPool::QueueWork(std::shared_ptr<Work> work)
	{
		std::lock_guard queueLock (m_workQueueMutex);
		m_workQueue.emplace_back(std::move(work));

		m_workReadyCondition.notify_all();
	}

	void ThreadPool::DoWorkParallel(std::shared_ptr<Work> work)
	{
		if (!work->MultiThreadedWork())
		{
			work->Execute();
			return;
		}

		{
			std::lock_guard queueLock (m_workQueueMutex);

			m_workQueue.emplace_back(std::move(work));

			m_workReadyCondition.notify_all();
		}

		// TODO: If there are other works in the queue, the main thread may end up running all the work. Is this the best approach? Maybe better than waiting for other works to finish
		while (!work->Finished())
		{
			work->Execute();
		}

		work->m_finishedCondition.notify_all();
	}

	void ThreadPool::WorkerThreadFunc(size_t thread_idx)
	{
		while (!m_stopWorkerThreads)
		{
			std::unique_lock lock(m_workQueueMutex);
			if (m_workQueue.empty())
			{
				m_workReadyCondition.wait(lock/*, [&]{ return !m_workQueue.empty();  }*/);
			}
			else
			{
				const std::shared_ptr<Work> work = m_workQueue.front();

				bool isRemoved = false;
				// Remove the Work from the queue if the Work will finish after the next 'Execute' call.
				// It is not removed from the queue if the Work will require more processing after that.
				if (work->WillFinishOnNextExecute())
				{
					isRemoved = true;
					m_workQueue.pop_front();
				}

				lock.unlock();

				work->Execute();

				lock.lock();

				if (work->Finished())
				{
					work->m_finishedCondition.notify_all();
					m_workReadyCondition.notify_all();
				}
			}
		}
	}
}
