#pragma once

#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <mutex>
#include <type_traits>
#include <queue>
#include <functional>

//#define DEBUG_POOL

#ifdef DEBUG_POOL
#include <QDebug>
#endif


class ThreadPool
{
    using Thread = std::thread;
    using Task = std::packaged_task<void()>;
    using TaskQueue = std::queue<Task>;

    ThreadPool(const ThreadPool&) = delete ;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void workerThread();

    ThreadPool(std::uint16_t sizePool);

public:

    static ThreadPool& pool()
    {
        static std::uint16_t numThreads = std::thread::hardware_concurrency() > 2 ? std::uint16_t(std::thread::hardware_concurrency() - 1) : 2;
        static ThreadPool threadPool(numThreads);
        return threadPool;
    }

    ~ThreadPool();

    template<typename Callable, typename... Args, typename ResultType = typename std::result_of<Callable(Args...)>::type>
    std::future<ResultType>
    submit(Callable&& f, Args&&... args);

    void wait();

    void processTasks();

private:
    std::atomic<std::uint16_t> m_countThreads;
    bool m_done;
    bool isWait;
    std::condition_variable m_condWakeWorker;
    std::condition_variable m_condWakePool;
    std::mutex m_mutex;
    TaskQueue m_taskQueue;
    std::atomic_int countTasks;
};


template<typename Callable, typename... Args, typename ResultType>
std::future<ResultType> ThreadPool::submit(Callable&& f, Args&&... args)
{
    std::packaged_task<ResultType()> task(std::bind(std::forward<Callable>(f),
                                                    std::forward<Args>(args)...));
    std::future<ResultType> res(task.get_future());

    {
    std::lock_guard<std::mutex> lg(m_mutex);
    m_taskQueue.push(Task(std::move(task)));
    }

    ++countTasks;

    m_condWakeWorker.notify_one();

    return res;
}
