#ifndef SJ_SERVER_H
#define SJ_SERVER_H

#include <assert.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "../config/config.h"
#include "../http/httpConnection.h"
#include "../common/util.h"
#include "../threadPool/threadPool.h"
#include "../timer/timer.h"

class WebServer {
public:
    WebServer();
    ~WebServer();

    //@description: enable the server listening function
    void server_listen();
    //@descrition: the main loop of server
    void server_loop();

    void init(int port, std::string usr, std::string passwd, std::string dbname, 
    int max_sql_num, int max_thread_num,  bool log_close, bool async_log, bool linger, bool cgi,
    TrigMode listen_trig_mode, TrigMode conn_trig_mode, ActorMode actor_mode);

    void log_init();
    void sql_pool_init();
    void thread_pool_init();

private:
    //@descrition：deal with the connection which is new arrival
    void deal_new_connection();
    //@description: deal with the connection which peet is shutdown
    void deal_shutdown_connection(int sock_fd);
    //@description: deal with the signal
    void deal_arrived_signal(int sock_fd);
    //@description: deal with the read request
    void deal_read_request(int conn_fd);
    //@description: deal with the write request
    void deal_write_request(int conn_fd);

public:
    bool m_time_out;
    bool m_stop_server;

    int m_port;
    std::string m_usr;
    std::string m_passwd;
    std::string m_dbname;
    int m_max_sql_num;
    int m_max_thread_num;
    
    bool m_log_close;
    bool m_async_log;
    bool m_linger;
    bool m_cgi;
    
    //监听文件描述符
    int m_listen_fd;
    //epoll描述符
    int m_epoll_fd;
    //pipe
    int m_pipe_fd[2];

    std::string m_root_path;

    TrigMode m_listen_trig_mode;
    TrigMode m_connect_trig_mode;
    ActorMode m_actor_mode;

    epoll_event m_events[MAX_EVENTS_NUMBER];
    HttpConnection *m_users;

    Timer m_timer;
};

#endif