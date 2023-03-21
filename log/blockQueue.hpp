#ifndef SJ_BLOCKQUEUE_H
#define SJ_BLOCKQUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <mutex>
#include <sys/time.h>
#include <semaphore.h>
#include <vector>

using namespace std;

template<typename T>
class blockQueue {
public:
    blockQueue(size_t max_size = 1000) {
        m_queue = vector<T>(max_size);
        m_max_size = max_size;
        m_size = 0;
        m_front = -1;
        m_tail = 0;
        sem_init(&m_full_buffers, 0, 0);
        sem_init(&m_empty_buffers, 0, max_size);
    }

    void clear() {
        m_mutex.lock();
        
        m_size = 0;
        m_front = -1;
        m_tail = 0;
        sem_init(&m_full_buffers, 0, 0);
        sem_init(&m_empty_buffers, 0, m_max_size);

        m_mutex.unlock();
    }

    bool empty() {
        bool is_empty = false;

        m_mutex.lock();
        if (m_size == 0)
            is_empty = true;
        m_mutex.unlock();

        return is_empty;
    }

    bool full() {
        bool is_full = false;

        m_mutex.lock();
        if (m_size >= m_max_size)
            is_full = true;
        m_mutex.unlock();

        return is_full;
    }

    int size() {
        return m_size;
    }

    void push(T &val) {
        sem_wait(&m_empty_buffers);

        m_mutex.lock();
        m_queue[m_tail] = move(val);
        m_tail = (m_tail + 1) % m_max_size;
        ++m_size; 
        m_mutex.unlock();

        sem_post(&m_full_buffers);
    }

    T peek() {
        m_mutex.lock();
        T &ret = m_queue[m_front];
        m_mutex.unlock();

        return ret;
    }

    T pop() {
        sem_wait(&m_full_buffers);

        m_mutex.lock();
        T& ret = m_queue[m_front];
        m_front = (m_front + 1) % m_max_size;
        --m_size;
        m_mutex.unlock();
        sem_post(&m_empty_buffers);
        return ret; 
    }

    

private:
    vector<T> m_queue;
    size_t m_max_size;
    size_t m_size;
    size_t m_front;
    size_t m_tail;
    mutex m_mutex;
    sem_t m_full_buffers;
    sem_t m_empty_buffers;
};

#endif