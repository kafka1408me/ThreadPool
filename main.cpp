#include <QDebug>
#include <QCoreApplication>
#include "ThreadPool.h"
#include <QString>
#include <QThread>

void freePrintHello()
{
    qDebug() << "Hello, user!";
}

int freeFunction(long min, long max)
{
    for(long i = min; i < max; ++i);
    qDebug() << "for end";
    return max - min;
}

class MyClass
{
public:
    MyClass()
    {
    }

    void printMsg(QString& msg)
    {
        QThread::sleep(5);
        qDebug() << msg;
        msg += " world!";
    }
};


void f(std::unique_ptr<int> val)
{
    QThread::sleep(3);
    qDebug() << "unique = " << *val;
}


template<unsigned SleepTime>
void templateFunc()
{
    QThread::sleep(SleepTime);
    qDebug() << "wake after " << SleepTime << " secs";
}



template<typename T> using  ToRvalue = typename std::remove_reference<typename std::remove_cv<T>::type>::type&&;


#define SUPER_MOVE(val) const_cast<ToRvalue<decltype(val)>>(val)


int main(int argc, char** argv)
{
    QCoreApplication a(argc, argv);

    long min = 1;
    long max = 9999999;

    MyClass myObj;

    QString str("Hello");

    qDebug() << str;

    // Get a reference to the pool
    ThreadPool& pool = ThreadPool::pool();

    // Add a free function to the task queue
    pool.submit(freePrintHello);

    // Add a free function that accepts 2 arguments to the task queue
    std::future<int> result = pool.submit(freeFunction, min, max); // or: auto result = ...


    // Add to task queue function-member of class; and pass reference
    pool.submit(&MyClass::printMsg, &myObj, std::ref(str));


    // Method to pass a non-copied object
    {
        const int value = 777;
        std::unique_ptr<int> uniqueVal(new int(value ));
        pool.submit([val = std::move(uniqueVal)]{f(SUPER_MOVE(val));});
    }

    // Add a lambda to task queue
    pool.submit([]{QThread::sleep(4); qDebug() << "lambda!";});

    // Add a template function to the queue
    pool.submit(templateFunc<2>);

    // Waiting for all queued pool tasks to complete
    // When processTasks() is called, the main thread also participates in processing queue tasks
    pool.processTasks(); // or pool.wait()

    qDebug() << "Tasks performed";

    qDebug() << "max-min = " << result.get();

    qDebug() << "changed str = " << str;

    return a.exec();
}
