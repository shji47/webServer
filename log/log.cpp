#include "log.h"

Log::Log() {
    m_cur_line = 0;
    m_log_index = 0;
}

Log::~Log() {
    if (m_fd)
        fclose(m_fd);
}

void Log::init(const char *dir_name, size_t max_line, size_t max_size, bool is_asyc, bool is_closed) {
    strcpy(m_dir_name, dir_name);
    m_max_line = max_line;
    m_is_aync = is_asyc;
    m_close_log = is_closed;

    if (is_asyc) {
        m_log_queue = new blockQueue<string>(max_size);
        thread th(write_log_handler);
        th.detach();
    }
    
    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);

    char file_name[FILE_NAME_LEN];
    memset(file_name, '\0', FILE_NAME_LEN);
    //dir/year_day_log.log
    snprintf(file_name, FILE_NAME_LEN, "%s%d_%02d_%02d_%s_%02ld%s", m_dir_name, sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, "log", m_log_index, ".log");
    ++m_log_index;
    
    m_cur_day = sys_tm->tm_mday;

    m_fd = fopen(file_name, "a");
    if (!m_fd) {
        cout<<"log file open failed!!!"<<endl;
        return;
    }
}

void Log::write_log(int level, const char *format, ...) {
    if (!m_fd) {
        cout<<"no valid log file"<<endl;
        return;
    }

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    char head_info[16] = {0};
    switch(level) {
    case 0:
        strcpy(head_info, "debug:");
        break;
    case 1:
        strcpy(head_info, "info:");
        break;
    case 2:
        strcpy(head_info, "warning:");
        break;
    case 3:
        strcpy(head_info, "error:");
        break;
    default:
        strcpy(head_info, "info:");
        break;
    }
    
    m_mutex.lock();
    // std::cout<<"lock1"<<std::endl;

    ++m_cur_line;

    if (m_cur_day != sys_tm->tm_mday || m_cur_line >= m_max_line) {
        m_cur_line = 0;
        if (m_cur_day != sys_tm->tm_mday) {
            m_log_index = 0;
            m_cur_day = sys_tm->tm_mday;
        }
        char file_name[FILE_NAME_LEN];
        fflush(m_fd);
        fclose(m_fd);

        snprintf(file_name, FILE_NAME_LEN, "%s%d_%02d_%02d_%s_%02ld%s", m_dir_name, sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, "log", m_log_index, ".log");
        ++m_log_index;

        m_fd = fopen(file_name, "a");
        if (!m_fd) {
            cout<<"new log file open failed!!!"<<endl;
            return;
        }
    }

    m_mutex.unlock();
    // std::cout<<"unlock1"<<std::endl;

    va_list valst;
    va_start(valst, format);

    m_mutex.lock();

    int len = snprintf(m_buf, BUF_SIZE, "%d-%02d-%02d %02d:%02d:%02d %s ",
    sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, sys_tm->tm_hour,
    sys_tm->tm_min, sys_tm->tm_sec, head_info);

    len += vsnprintf(m_buf + len, BUF_SIZE - len, format, valst);

    m_buf[len++] = '\n';
    m_buf[len++] = '\0';

    m_mutex.unlock();

    if (m_is_aync) {
        m_log_queue->push(string().assign(m_buf, len));
    }
    else {
        m_mutex.lock();
        
        fputs(m_buf, m_fd);
        m_mutex.unlock();
    }

    va_end(valst);
}