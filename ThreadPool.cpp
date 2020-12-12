#include "ThreadPool.h"
//#include <QDebug>

#ifdef DEBUG_POOL
#include <QThread>
#endif


ThreadPool::ThreadPool(std::uint16_t sizePool):
    m_countThreads(0),
    m_done(false),
    isWait(false),
    countTasks(0)
{
#ifdef DEBUG_POOL
    qDebug() << QString("ThreadPool: construct pool with creating %1 worker threads").arg(sizePool);
#endif
    for(int i = 0; i < sizePool; ++i)
    {
        Thread t(&ThreadPool::workerThread, this);
        t.detach();
    }
}


ThreadPool::~ThreadPool()
{
#ifdef DEBUG_POOL
    qDebug() << "ThreadPool::~ThreadPool()";
#endif
    m_done = true;
    std::unique_lock<std::mutex> lk(m_mutex);
    m_condWakeWorker.notify_all();
    m_condWakePool.wait(lk, [this] { return !m_countThreads; } );
#ifdef DEBUG_POOL
    qDebug() << "ThreadPool::~ThreadPool() end";
#endif
}

void ThreadPool::wait()
{
#ifdef DEBUG_POOL
    qDebug() << "ThreadPool: start wait";
#endif
    isWait = true;
    std::unique_lock<std::mutex> lk(m_mutex);
    m_condWakePool.wait(lk,[&]() {return !countTasks;});
    isWait = false;
#ifdef DEBUG_POOL
    qDebug() << "ThreadPool: end wait";
#endif
}

void ThreadPool::processTasks()
{
    isWait = true;
    std::unique_lock<std::mutex> lk(m_mutex);

#ifdef DEBUG_POOL
    qDebug() << "ThreadPool: start processTasks";
#endif

    while(countTasks)
    {
        if(!lk)
        {
            lk.lock();
        }
        if(!m_taskQueue.empty())
        {
            Task task = std::move(m_taskQueue.front());
            m_taskQueue.pop();

            lk.unlock();

            task();

#ifdef DEBUG_POOL
            qDebug() << "ThreadPool: task completed; MainThread";
#endif

            --countTasks;
        }
        else
        {
            m_condWakePool.wait(lk, [this]{return !countTasks;});
        }
    }
    isWait = false;
#ifdef DEBUG_POOL
    qDebug() << "ThreadPool: end processTasks";
#endif
}


void ThreadPool::workerThread()
{
    ++m_countThreads;

#ifdef DEBUG_POOL
    auto id = QThread::currentThreadId();

#endif
    std::unique_lock<std::mutex> lk(m_mutex);

    while(!m_done)
    {
        if(!m_taskQueue.empty())
        {
            Task task = std::move(m_taskQueue.front());
            m_taskQueue.pop();

            lk.unlock();

            task();

#ifdef DEBUG_POOL
            qDebug() << "ThreadPool: task completed; id = " << id;
#endif

            --countTasks;

            if(isWait && !countTasks)
            {
                m_condWakePool.notify_one();
            }

        }

        if(!lk)
        {
            lk.lock();
        }

        m_condWakeWorker.wait(lk, [this]{ return !m_taskQueue.empty() || m_done; });
    }

#ifdef DEBUG_POOL
    qDebug() << "ThreadPool: worker Off; id = " << id;
#endif
    --m_countThreads;
    m_condWakePool.notify_one();
}
