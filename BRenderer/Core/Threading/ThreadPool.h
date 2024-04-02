#ifndef BRR_ThreadPool_h
#define BRR_ThreadPool_h
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <atomic>

namespace brr::thread
{
	class Work;

	class ThreadPool
	{
	public:

		static ThreadPool& GetDefaultPool();

		ThreadPool();
		ThreadPool(size_t num_threads);

		ThreadPool(ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;

		ThreadPool& operator=(ThreadPool&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		~ThreadPool();

		/**
		 * \brief Queue a work in the work queue, where a thread from the thread pool will execute the work.
		 * \param work A shared_ptr of the work to be executed.
		 */
		void QueueWork(std::shared_ptr<Work> work);

		/**
		 * \brief Queue a work in the work queue, and also uses the main thread to help the pool with the work execution. The function only returns when the work is finished.
		 * If the Work is not MultiThreaded (i.e., can be divided into multiple chunks of work), then the main thread will execute it and return.
		 * \param work A shared_ptr of the work to be executed.
		 */
		void DoWorkParallel(std::shared_ptr<Work> work);

		[[nodiscard]] constexpr size_t AvailableWorkers() const { return (m_workerThreads.size() - m_runningThreads); }

	private:

		void WorkerThreadFunc(size_t thread_idx);

		std::list<std::shared_ptr<Work>> m_workQueue{};
		std::vector<std::thread> m_workerThreads{};

		size_t m_runningThreads;

		std::mutex m_workQueueMutex;
		std::condition_variable m_workReadyCondition;

		std::atomic<bool> m_stopWorkerThreads;
	};

}

#endif