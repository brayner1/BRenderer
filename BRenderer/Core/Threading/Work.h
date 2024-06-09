#ifndef BRR_Work_h
#define BRR_Work_h
#include <mutex>
#include <functional>
#include <iostream>

namespace brr::thread
{
	class Work
	{
		friend class ThreadPool;
	public:
		Work(bool multi_threaded = false) : m_dividibleInChunks(multi_threaded), m_finished(false) { }

		virtual ~Work() = default;

		virtual void Execute() = 0;

		virtual bool WillFinishOnNextExecute() = 0;
		/**
		 * \brief Works that are multi-threaded can be divided in chunks and computed by multiple threads in the ThreadPool.
		 */
		[[nodiscard]] bool MultiThreadedWork() const { return m_dividibleInChunks; }

		[[nodiscard]] bool Finished() const { return m_finished; };

		//TODO: Add Wait function  (Maybe 'WaitStart', 'WaitFinish', ... ?)
		void Wait()
		{
			std::unique_lock<std::mutex> lock(m_workMutex);

			if (m_finished)
				return;

			m_finishedCondition.wait(lock, [&] { return m_finished.load(); });
		}

	protected:
		std::mutex m_workMutex;
		std::condition_variable m_finishedCondition;

		const bool m_dividibleInChunks;
		std::atomic_bool m_finished;
	};

	template<typename... Args>
	class FunctionWork final : public Work
	{
		using Func = std::function<void(Args...)>;
	public:
		explicit FunctionWork(Func&& function, Args&&... args)
			: function(std::forward<Func>(function)), args{std::forward<Args>(args)...}
		{}

		void Execute() override
		{
			std::apply(function, args);

			m_finished = true;
		}

		bool WillFinishOnNextExecute() override
		{
			return true;
		}

		~FunctionWork() override = default;

	private:
		Func function;
		std::tuple<Args...> args;
	};

	template<typename T>
	class ForLoopWork final : public Work
	{
	public:
		using iterator = T;
		using LoopFunc = std::function<void(iterator)>;

		ForLoopWork(iterator end_iter, size_t chunk_size, LoopFunc&& loop_func)
		: ForLoopWork(0, end_iter, chunk_size, std::forward<LoopFunc>(loop_func)) {}

		ForLoopWork(iterator start_iter, iterator end_iter, size_t chunk_size, LoopFunc&& loop_func)
		: Work((end_iter - start_iter) > chunk_size),
		m_currentIter(start_iter), m_endIter(end_iter), m_loopFunc(std::move(loop_func)),
		m_chunkSize(std::min(chunk_size, static_cast<size_t>(end_iter - start_iter))), m_numThreads(0)
		{}
		
		~ForLoopWork() override { std::cout << "destructing for loop work\n"; }

		void Execute() override
		{
			std::unique_lock<std::mutex> lock (loopMutex);
			while (m_currentIter != m_endIter)
			{
				iterator startIt = m_currentIter;
				iterator endIt = std::min(m_currentIter + m_chunkSize, m_endIter);
				m_currentIter = endIt;

				m_numThreads++;

				lock.unlock();

				for (iterator it = startIt; it < endIt; ++it)
				{
					m_loopFunc(it);
				}

				lock.lock();

				m_numThreads--;

				// The work is only finished once the number of iterations ends,
				// and there is no more threads working on any of other iterations.
				// (Note that the thread that picked the last iterations might finish before a
				// thread that picked previous loop iterations, thus requiring the check for the number of threads working before finishing.)
				if (m_currentIter == m_endIter && m_numThreads == 0)
					m_finished = true;
			}
		}

		bool WillFinishOnNextExecute() override
		{
			iterator currIt;
			{
				std::scoped_lock<std::mutex> lock (loopMutex);
				currIt = m_currentIter;
			}
			return std::min(currIt + m_chunkSize, m_endIter) == m_endIter;
		}

	private:
		iterator m_currentIter;
		const iterator m_endIter;
		const size_t m_chunkSize;
		size_t m_numThreads;
		std::mutex loopMutex;
		LoopFunc m_loopFunc;
	};

	class ForLoop2DWork final : public Work
	{
	public:
		using iterator = std::pair<size_t, size_t>;
		using Loop2DFunc = std::function<void(std::pair<size_t,size_t>)>;

		ForLoop2DWork(std::pair<size_t, size_t> end_iter_pair, std::pair<size_t,size_t> chunk_size, Loop2DFunc&& loop_func)
			: ForLoop2DWork({0,0}, end_iter_pair, chunk_size, std::forward<Loop2DFunc>(loop_func)) {}

		ForLoop2DWork(std::pair<size_t, size_t> start_iter_pair, 
			std::pair<size_t, size_t> end_iter_pair, 
			std::pair<size_t,size_t> chunk_size, Loop2DFunc&& loop_func) :
			Work(true/*chunk_size.first < static_cast<size_t>(end_iter_pair.first - start_iter_pair.first) 
				|| chunk_size.second < static_cast<size_t>(end_iter_pair.second - start_iter_pair.second)*/),
			m_currentIter(start_iter_pair), m_endIter(end_iter_pair),
			m_chunkSize(std::min(chunk_size.first, static_cast<size_t>(end_iter_pair.first - start_iter_pair.first)), 
						std::min(chunk_size.second, static_cast<size_t>(end_iter_pair.second - start_iter_pair.second))),
			m_loopFunc(std::move(loop_func)), m_numThreads(0)
		{
			/*m_chunkSize = {
				std::min(chunk_size.first, static_cast<size_t>(end_iter_pair.first - start_iter_pair.first)), 
				std::min(chunk_size.second, static_cast<size_t>(end_iter_pair.second - start_iter_pair.second))
			};*/

			//m_chunkSize(std::min(chunk_size, static_cast<size_t>(end_iter - start_iter))), 
		}

		void Execute() override
		{
			std::unique_lock<std::mutex> lock (loopMutex);
			while (m_currentIter != m_endIter)
			{
				iterator startIt = m_currentIter;
				iterator endIt = iterator{
					std::min<size_t>(m_currentIter.first + m_chunkSize.first, m_endIter.first),
					std::min<size_t>(m_currentIter.second + m_chunkSize.second, m_endIter.second)
				};
				m_currentIter = endIt;

				m_numThreads++;

				lock.unlock();

				for (size_t vert_it = startIt.second; vert_it < endIt.second; ++vert_it)
				{
					for (size_t hor_it = startIt.first; hor_it < endIt.first; ++hor_it)
					{
						m_loopFunc({hor_it, vert_it});
					}
				}

				lock.lock();

				m_numThreads--;

				// The work is only finished once the number of iterations ends,
				// and there is no more threads working on any of other iterations.
				// (Note that the thread that picked the last iterations might finish before a
				// thread that picked previous loop iterations, thus requiring the check for the number of threads working before finishing.)
				if (m_currentIter == m_endIter && m_numThreads == 0)
					m_finished = true;
			}
		}

		bool WillFinishOnNextExecute() override
		{
			iterator curr_it;
			{
				std::scoped_lock<std::mutex> lock (loopMutex);
				curr_it = m_currentIter;
			}
			const iterator next_it = iterator{
					std::min<size_t>((curr_it.first + m_chunkSize.first), m_endIter.first),
					std::min<size_t>((curr_it.second + m_chunkSize.second), m_endIter.second)
				};
			return next_it == m_endIter;
		}

	private:
		std::pair<size_t,size_t> m_currentIter;
		const std::pair<size_t,size_t> m_endIter;
		const std::pair<size_t,size_t> m_chunkSize;
		size_t m_numThreads;
		std::mutex loopMutex;
		Loop2DFunc m_loopFunc;
	};  
}

#endif