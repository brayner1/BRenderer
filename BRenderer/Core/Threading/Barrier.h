#ifndef BRR_Barrier_h
#define BRR_Barrier_h
#include <mutex>


namespace brr::threading
{
	class Barrier
	{
	public:
		explicit Barrier (size_t numThreads) :
		originalExpected(numThreads), expectedNum(numThreads), generation(false),
		mutex(std::mutex()), cv(std::condition_variable())
		{
		}

		Barrier(const Barrier& ) = delete;
		Barrier& operator=(const Barrier&) = delete;

		void Wait()
		{
			std::unique_lock lock(mutex);
			size_t lastGen = generation;
			if (--expectedNum == 0)
			{
				// Reset the barrier after this generation completed.
				generation = !generation;
				expectedNum = originalExpected;
				// Notify the rest of the threads
				cv.notify_all();
			}
			else
			{
				// Wait inside the barrier until this generation is complete.
				cv.wait(lock, [&]() { return lastGen != generation; });
			}
		}

	private:
		std::mutex mutex;
		std::condition_variable cv;
		size_t originalExpected;
		size_t expectedNum;
		bool generation;
	};

}

#endif