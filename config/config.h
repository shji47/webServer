#ifndef SJ_CONFIG_H
#define SJ_CONFIG_H

//用于各种系统参数配置
//单例

#include "../common/util.h"

class Config {
private:
    Config() {}
    ~Config() {}

public:
    static Config* singleton() {
        static Config* cfg = new Config();
        return cfg;
    }

public:
    int m_server_port = 9999;
    bool m_async_log = true;
    bool m_close_log = true;
    bool m_linger = false;
    bool m_cgi = false;
    TrigMode m_listen_trig_mode = TrigMode::ET;
    TrigMode m_conn_trig_mode = TrigMode::ET;
    ActorMode m_actor_mode = ActorMode::PROACTOR;
    int m_max_sql_num = 8;
    int m_max_thread_num = 8;
    
    unsigned int m_time_slot;
};

#endif