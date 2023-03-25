#ifndef SJ_LOG_H
#define SJ_LOG_H

#include <ctime>
#include <string>
#include <string.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>

#include "blockQueue.hpp"
#include "../common/util.h"

using namespace std;

class Log {
public:
    static Log* singleton() {
        static Log instance;
        return &instance;
    }

    void init(const char *dir_name, size_t max_line, size_t max_size, bool is_asyc, bool is_closed);
    void write_log(int level, const char *format, ...);
    

    void flush() {
        m_mutex.lock();
        // std::cout<<"flush lock"<<std::endl;
        fflush(m_fd);
        m_mutex.unlock();
        // std::cout<<"flush unlock"<<std::endl;
    }

    static void write_log_handler() {
        Log::singleton()->write_log_asyc();
    }

    bool close() {return m_close_log;};

private:
    Log();
    ~Log();

    void write_log_asyc() {
        string log_info;
        while (true) {
            // while (m_log_queue->empty());
            log_info = move(m_log_queue->pop());
            // std::cout<<log_info<<std::endl;
            m_mutex.lock();
            fputs(log_info.c_str(), m_fd);
            fflush(m_fd);
            m_mutex.unlock();
        }
    }

    //日志输出缓存
    char m_buf[BUF_SIZE];
    //日志文件存储目录
    char m_dir_name[FILE_NAME_LEN];
    //单个日志文件最大行数
    uint32_t m_max_line;
    //当前日志行数
    uint32_t m_cur_line;
    //最新更新时的日期，日志文件按天划分
    int m_cur_day;
    //缓冲区锁，避免多线程情况下缓冲区输出乱序
    mutex m_mutex;
    //日志文件描述符
    //还是使用C风格的文件操作
    //大数据量的情况下，C比C++写操作快四倍
    //参考链接：https://blog.csdn.net/shudaxia123/article/details/50491451
    FILE *m_fd;
    //异步情况下日志队列
    blockQueue<string> *m_log_queue;
    //是否是异步日志
    bool m_is_aync;
    //是否关闭日志
    bool m_close_log;
    //日志文件index
    size_t m_log_index;
};


#define LOG_DEBUG(format, ...) if (!Log::singleton()->close()) {Log::singleton()->write_log(0, format, ##__VA_ARGS__); Log::singleton()->flush();}
#define LOG_INFO(format, ...) if (!Log::singleton()->close()) {Log::singleton()->write_log(1, format, ##__VA_ARGS__); Log::singleton()->flush();}
#define LOG_WARNING(format, ...) if (!Log::singleton()->close()) {Log::singleton()->write_log(2, format, ##__VA_ARGS__); Log::singleton()->flush();}
#define LOG_ERROR(format, ...) if (!Log::singleton()->close()) {Log::singleton()->write_log(3, format, ##__VA_ARGS__); Log::singleton()->flush();}

#endif