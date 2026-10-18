#pragma once
#include <queue>
#include <thread>
#include <mutex> //pthread_mutex_x
#include <condition_variable> //pthread_condition_t

//线程安全的阻塞队列，为Logger提供异步日志缓冲
template<typename T>
class LockQueue
{
private:
    //数据容器，存放待处理日志
    std::queue<T> m_queue;
    //互斥锁，保存队列的读写
    std::mutex m_mutex;
    //条件变量，通知消费者有新数据
    std::condition_variable m_condvariable;
public:
    void push(const T& data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_condvariable.notify_one();
    }
    //目前只有一个线程写日志queue，写日志文件
    T pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condvariable.wait(lock, [this]{
            return !m_queue.empty();
        });
        T data = std::move(m_queue.front());
        m_queue.pop();
        return data;
    }
};