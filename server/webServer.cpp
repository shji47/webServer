#include "webServer/server/webServer.h"
#include "webServer.h"

void WebServer::server_listen() {
    Config *cfg = Config::singleton();
    Utils util;

    //网络编程基础
    m_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(m_listen_fd != -1);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(cfg->m_server_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret = bind(m_listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    assert(ret != -1);

    ret = listen(m_listen_fd, cfg->m_max_connection);
    assert(ret != -1);

    //size参数用于告诉内核epoll会处理的事件大致数目，而不是其能处理的最大数
    m_epoll_fd = epoll_create(5);

    util.epoll_add_fd(m_epoll_fd, m_listen_fd, false, cfg->m_mode);
    
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipe_fd);
    util.set_nonblocking(m_pipe_fd[1]);
    util.epoll_add_fd(m_epoll_fd, m_pipe_fd[0], false, TrigMode::LT);

    //将SIGPIPE信号忽略，原因:https://blog.csdn.net/chengcheng1024/article/details/108104507
    util.add_sig(SIGPIPE, SIG_IGN, true);
    util.add_sig(SIGALRM, Utils::sig_handler, false);
    util.add_sig(SIGTERM, Utils::sig_handler, false);

    alarm(cfg->m_time_slot);

    Utils::m_pipe_fd = m_pipe_fd;
    Utils::m_epoll_fd = m_epoll_fd;
}

void WebServer::server_loop() {
    m_time_out = false;
    m_stop_server = false;

    while (!m_stop_server) {
        int events_num = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS_NUMBER, -1);
        if (events_num < 0 && errno != EINTR) {
            break;
        }

        for (int i = 0; i < events_num; ++i) {
            int sock_fd = m_events[i].data.fd;
            //listening fd
            //new connection arrived
            if (sock_fd == m_listen_fd) {
                deal_new_connection();
            }
            //peer normal/abnormal shutdown
            else if (m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                deal_shutdown_connection();
            }
            else if ((sock_fd == m_pipe_fd[0]) || (m_events[i].events & EPOLLIN)) {
                deal_arrived_signal(m_time_out, m_stop_server);
            }
            else if (m_events[i].events & EPOLLIN) {
                deal_read_request(sock_fd);
            }
            else if (m_events[i].events & EPOLLOUT) {
                deal_write_request(sock_fd);
            }
        }
    }
}

void WebServer::deal_new_connection()
{
    
}

void WebServer::deal_shutdown_connection()
{

}

void WebServer::deal_arrived_signal(bool& timeout, bool& server_stop) {

}

void WebServer::deal_read_request(int conn_fd)
{
}

void WebServer::deal_write_request(int conn_fd)
{
}
