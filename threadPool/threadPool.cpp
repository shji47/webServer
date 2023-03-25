#ifndef SJ_THREADPOOL_H
#define SJ_THREADPOOL_H

#include "threadPool.h"
threadPool* threadPool::m_instance = nullptr;
std::mutex threadPool::m_mutex;

void threadPool::init(int max_thread, int max_task, ActorMode actor_mode) {
    m_max_thread = max_thread;
    m_max_task = max_task;
    m_actor_mode = actor_mode;

    m_task_queue = new blockQueue<HttpConnection*>(max_task);
    m_threads = new std::thread*[max_thread];

    for (int i = 0; i < max_thread; ++i) {
        m_threads[i] = new std::thread(worker, this);
        m_threads[i]->detach();
    }
} 

void threadPool::add_task(HttpConnection *task) {
    m_task_queue->push(task);
}

void threadPool::run() {
    while (true) {
        HttpConnection *task = m_task_queue->pop();
        if (m_actor_mode == ActorMode::PROACTOR) {
            mysqlRAII mysql_raii(&task->m_mysql);
            task->process();
        }
        else if (m_actor_mode == ActorMode::REACTOR) {
            if (task->m_state == 0) {
                if (task->read()) {
                    mysqlRAII mysql_raii(&task->m_mysql);
                    task->process();
                }
                else {
                    task->m_kill = true;
                }
                task->m_has_deal = true;
            }
            else if (task->m_state == 1) {
                if (!task->write()) {
                    task->m_kill = true;
                }
                task->m_has_deal = true;
            }
        }
    }
}


void threadPool::worker(threadPool *pool) {
    pool->run();
}

#endif