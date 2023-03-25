#include "mysqlPool.h"

mysqlPool::mysqlPool() {
    m_used_connection = 0;
    m_free_connection = 0;
}

mysqlPool* mysqlPool::singleton() {
    static mysqlPool instance;
    return &instance;
}

void mysqlPool::init(std::string host, std::string user, std::string passwd,\
std::string dbname, int max_connection, unsigned int port) {
    m_host = std::move(host);
    m_port = port;
    m_user = std::move(user);
    m_passwd = std::move(passwd);
    m_dbname = std::move(dbname);

    for (int i = 0; i < max_connection; ++i) {
        MYSQL *conn;

        if (!(conn = mysql_init(conn))) {
            std::cout<<"Mysql Error: "<<mysql_error(conn)<<std::endl;
            exit(1);
        }
        
        // conn = mysql_real_connect(conn, m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), port, NULL, 0);
        
        if (!(conn = mysql_real_connect(conn, m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), port, NULL, 0))) {
            std::cout<<"Mysql Error: "<<mysql_error(conn)<<std::endl;
            exit(1);
        }
        m_conn_list.push_back(conn);
        ++m_free_connection;
    }

    m_max_connection = m_free_connection;
    sem_init(&m_reserve, 0, m_max_connection);
}

MYSQL* mysqlPool::getConnection() {
    if (m_conn_list.size() <= 0)
        return nullptr;
    
    //消耗一个链接
    sem_wait(&m_reserve);
    m_mutex.lock();
    MYSQL *conn = m_conn_list.front();
    m_conn_list.pop_front();

    --m_free_connection;
    ++m_used_connection;

    m_mutex.unlock();

    return conn;
}

bool mysqlPool::realeaseConnection(MYSQL* conn) {
    if (!conn)
        return false;

    m_mutex.unlock();

    m_conn_list.emplace_back(conn);
    --m_used_connection;
    ++m_free_connection;

    m_mutex.unlock();

    sem_post(&m_reserve);

    return true;
}

mysqlPool::~mysqlPool() {
    m_mutex.lock();

    for (auto iter = m_conn_list.begin(); iter != m_conn_list.end(); ++iter) {
        mysql_close(*iter);
    }
    m_used_connection = 0;
    m_free_connection = 0;
    m_conn_list.clear();

    m_mutex.unlock();
}

mysqlRAII::mysqlRAII(MYSQL **conn) {
    m_conn = mysqlPool::singleton()->getConnection();
    *conn = m_conn;
}

mysqlRAII::~mysqlRAII() {
    mysqlPool::singleton()->realeaseConnection(m_conn);
}