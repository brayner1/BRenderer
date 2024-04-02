#include "MainThread.h"

namespace brr::thread
{

    std::list<MainThread::FunctionType> MainThread::m_main_tasks = {};
    std::mutex MainThread::m_task_mutex = {};

    void MainThread::RunOnMain(const FunctionType& function)
    {
        std::lock_guard<std::mutex> lock (m_task_mutex);
        m_main_tasks.push_back(function);
    }

    void MainThread::RunOnMain(FunctionType&& function)
    {
        std::lock_guard<std::mutex> lock (m_task_mutex);
        m_main_tasks.emplace_back(std::move(function));
    }

    void MainThread::RunMain()
    {
        std::lock_guard<std::mutex> lock (m_task_mutex);
        for (FunctionType& function : m_main_tasks)
        {
            function();
        }
        m_main_tasks.clear();
    }
}
