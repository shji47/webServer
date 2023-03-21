#ifndef SJ_CONFIG_H
#define SJ_CONFIG_H

//用于各种系统参数配置
//单例

#include "webServer/common/util.h"

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
    int m_server_port;
    int m_max_connection;
    TrigMode m_mode;
    unsigned int m_time_slot;
};

#endif