#ifndef SJ_MYSQLPOOL_H
#define SH_MYSQLPOOL_H

#include <iostream>
#include <mysql/mysql.h>
#include <mutex>
#include <string>
#include <list>
#include <semaphore.h>

class mysqlPool {
public:
    static mysqlPool* singleton();

    MYSQL* getConnection();
    bool realeaseConnection(MYSQL *sql);

    void init(std::string host, std::string user, std::string passwd,
    std::string dbname, int max_connection, unsigned int port);


private:
    mysqlPool();
    ~mysqlPool();

    //最大连接数
    int m_max_connection;
    //使用连接数
    int m_used_connection;
    //空闲连接数
    int m_free_connection;

    std::mutex m_mutex;
    
    //连接链表
    std::list<MYSQL*> m_conn_list;
    sem_t m_reserve;

public:
    std::string m_host;
    std::string m_user;
    std::string m_passwd;
    std::string m_dbname;
    unsigned int m_port;
};

class mysqlRAII {
public:
    mysqlRAII(MYSQL **conn);
    ~mysqlRAII();

private:
    MYSQL *m_conn;
};


#endif