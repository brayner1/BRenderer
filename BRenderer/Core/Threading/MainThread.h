#ifndef BRR_MAINTHREAD_H
#define BRR_MAINTHREAD_H

#include <list>
#include <functional>
#include <mutex>

namespace brr::thread
{

    class MainThread
    {
    public:

        using FunctionType = std::function<void()>;

        static void RunOnMain(const FunctionType& function);

        static void RunOnMain(FunctionType&& function);

        static void RunMain();

    private:

        static std::list<FunctionType> m_main_tasks;
        static std::mutex m_task_mutex;
    };

}

#endif