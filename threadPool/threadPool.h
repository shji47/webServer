#ifndef SJ_THREAD_POOL
#define SJ_THREAD_POOL
#include <thread>
#include <mutex>

#include "../http/httpConnection.h"
#include "../log/blockQueue.hpp"
#include "../common/util.h"
#include "../mysqlPool/mysqlPool.h"

class threadPool {
public:
    static threadPool* singleton(){
        //好想会和Log产生竞争
        // static threadPool instance;
        // return &instance;
        if (m_instance == nullptr) {
            m_mutex.lock();
            if (m_instance == nullptr) {
                m_instance = new threadPool();
            }
            m_mutex.unlock();
        }

        return m_instance;
    }

    void init(int max_thread, int max_task, ActorMode actor_mode);
    void add_task(HttpConnection *task);

private:
    threadPool() {}
    ~threadPool() {
        for (int i = 0; i < m_max_thread; ++i) {
            delete m_threads[i];
        }
        delete[] m_threads;
    }

    static void worker(threadPool *pool);
    void run();

private:
    ActorMode m_actor_mode;
    blockQueue<HttpConnection*> *m_task_queue;
    std::thread **m_threads;
    int m_max_thread;
    int m_max_task;
    static std::mutex m_mutex;
    static threadPool *m_instance;
};

#endif