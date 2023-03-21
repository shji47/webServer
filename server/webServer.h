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

#include "webServer/config/config.h"
#include "webServer/http/httpConnection.h"

//最大连接数
#define MAX_CONNECTION 10000


const int MAX_EVENTS_NUMBER = 10000;

class WebServer {
public:
    WebServer() {}
    ~WebServer() {}

    //@description: enable the server listening function
    void server_listen();
    //@descrition: the main loop of server
    void server_loop();
    //@descrition：deal with the connection which is new arrival
    void deal_new_connection();
    //@description: deal with the connection which peet is shutdown
    void deal_shutdown_connection();
    //@description: deal with the signal
    void deal_arrived_signal(bool& timeout, bool& server_stop);
    //@description: deal with the read request
    void deal_read_request(int conn_fd);
    //@description: deal with the write request
    void deal_write_request(int conn_fd);

public:
    bool m_time_out;
    bool m_stop_server;
    
    //监听文件描述符
    int m_listen_fd;
    //epoll描述符
    int m_epoll_fd;
    //pipe
    int m_pipe_fd[2];

    epoll_event m_events[MAX_EVENTS_NUMBER];
    HttpConnection m_conns[MAX_CONNECTION];
};

#endif