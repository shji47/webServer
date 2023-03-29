#include "../server/webServer.h"
#include "../log/log.h"
#include "webServer.h"

WebServer::WebServer() {
    m_users = new HttpConnection[MAX_CONNECTION];

    char server_path[FILE_NAME_LEN];
    getcwd(server_path, FILE_NAME_LEN);
    char root[] = "/root";
    
    m_root_path = "";
    m_root_path.append(server_path).append(root);
}

WebServer::~WebServer() {
    close(m_listen_fd);
    close(m_epoll_fd);
    close(m_pipe_fd[0]);
    close(m_pipe_fd[1]);

    delete[] m_users;
}

//初始化

void WebServer::init(int port, std::string usr, std::string passwd, std::string dbname, 
    int max_sql_num, int max_thread_num,  bool log_close, bool async_log, bool linger, bool cgi,
    TrigMode listen_trig_mode, TrigMode conn_trig_mode, ActorMode actor_mode) {
    m_port = port;
    m_usr = std::move(usr);
    m_passwd = std::move(passwd);
    m_dbname = std::move(dbname);
    m_max_sql_num = max_sql_num;
    m_max_thread_num = max_thread_num;
    m_log_close = log_close;
    m_async_log = async_log;
    m_linger = linger;
    m_cgi = cgi;
    m_listen_trig_mode = listen_trig_mode;
    m_connect_trig_mode = conn_trig_mode;
    m_actor_mode = actor_mode;
}

void WebServer::log_init() {
    Log::singleton()->init("./record/", 800000, 3000, m_async_log, m_log_close);
    LOG_INFO("\n\n\n\n\n****************************************");
    LOG_INFO("Log System Running...");
}

void WebServer::sql_pool_init() {
    mysqlPool::singleton()->init("localhost", m_usr, m_passwd, m_dbname, m_max_sql_num, 3306);

    MYSQL *sql = nullptr;
    mysqlRAII sql_raii(&sql);
    if (!sql) {
        LOG_ERROR("Mysql Error: get connection failed\n");
        return ;
    }
    
    if (mysql_query(sql, "SELECT username,passwd FROM usrInfo")) {
        cout<<"Mysql Error: select error: "<<mysql_error(sql)<<endl;
        LOG_ERROR("Mysql Error: select error: %s\n", mysql_error(sql));
        return ;
    }

    MYSQL_RES *result = mysql_store_result(sql);

    int num_files = mysql_num_fields(result);
    
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        HttpConnection::usr_infos.insert(std::pair<std::string, std::string>(row[0], row[1]));
    }
    LOG_INFO("Sql Pool System Running...");
}

void WebServer::thread_pool_init() {
    threadPool::singleton()->init(m_max_thread_num, 50000, m_actor_mode);
    LOG_INFO("Thread Pool System Running...");
}

//业务
void WebServer::server_listen() {
    Config *cfg = Config::singleton();

    //网络编程基础
    m_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(m_listen_fd != -1);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret = bind(m_listen_fd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    assert(ret != -1);

    ret = listen(m_listen_fd, 5);
    assert(ret != -1);

    //size参数用于告诉内核epoll会处理的事件大致数目，而不是其能处理的最大数
    m_epoll_fd = epoll_create(5);

    HttpConnection::epoll_fd = m_epoll_fd;
    Utils::m_epoll_fd = m_epoll_fd;
    Utils::epoll_add_fd(m_epoll_fd, m_listen_fd, false, m_listen_trig_mode);
    
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipe_fd);
    Utils::set_nonblocking(m_pipe_fd[1]);
    Utils::epoll_add_fd(m_epoll_fd, m_pipe_fd[0], false, TrigMode::LT);

    //将SIGPIPE信号忽略，原因:https://blog.csdn.net/chengcheng1024/article/details/108104507
    Utils::add_sig(SIGPIPE, SIG_IGN, true);
    Utils::add_sig(SIGALRM, Utils::sig_handler, false);
    Utils::add_sig(SIGTERM, Utils::sig_handler, false);

    alarm(cfg->m_time_slot);
    m_timer.init(cfg->m_time_slot);

    Utils::m_pipe_fd = m_pipe_fd;
}

void WebServer::server_loop() {
    m_time_out = false;
    m_stop_server = false;

    while (!m_stop_server) {
        int events_num = epoll_wait(m_epoll_fd, m_events, MAX_EVENTS_NUMBER, -1);
        if (events_num < 0 && errno != EINTR) {
            break;
        }

        // std::cout<<events_num<<std::endl;

        for (int i = 0; i < events_num; ++i) {
            int sock_fd = m_events[i].data.fd;
            //listening fd
            //new connection arrived
            if (sock_fd == m_listen_fd) {
                deal_new_connection();
            }
            //peer normal/abnormal shutdown
            else if (m_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                deal_shutdown_connection(sock_fd);
            }
            else if ((sock_fd == m_pipe_fd[0]) && (m_events[i].events & EPOLLIN)) {
                deal_arrived_signal(sock_fd);
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
    struct sockaddr_in conn_addr;
    socklen_t len = sizeof(conn_addr);
    int conn_fd;
    
    if (m_listen_trig_mode == TrigMode::LT) {
        if ((conn_fd = accept(m_listen_fd, (struct sockaddr*) &conn_addr, &len)) < 0) {
            LOG_ERROR("Accept error: errno is %d", errno);
            return ;
        }
        if (HttpConnection::user_count >= MAX_CONNECTION) {
            char busy_info[] = "Internal Server busy";
            LOG_ERROR("%s", busy_info);
            send(conn_fd, busy_info, strlen(busy_info), 0);
            close(conn_fd);
            return ;
        }
        LOG_INFO("Accept new connecton: %d", conn_fd);
        m_users[conn_fd].init(conn_fd, conn_addr, m_root_path, m_connect_trig_mode, m_cgi);
        m_timer.add_sock(conn_fd);
    }
    else if (m_listen_trig_mode == TrigMode::ET) {
        while (true) {
            if ((conn_fd = accept(m_listen_fd, (struct sockaddr*) &conn_addr, &len)) < 0) {
                if (errno != EAGAIN)
                    LOG_ERROR("Accept error: errno is %d", errno);
                return ;
            }
            if (HttpConnection::user_count >= MAX_CONNECTION) {
                char busy_info[] = "Internal Server busy";
                LOG_ERROR("%s", busy_info);;
                send(conn_fd, busy_info, strlen(busy_info), 0);
                close(conn_fd);
                return ;
            }
            LOG_INFO("Accept new connecton: %d", conn_fd);
            m_users[conn_fd].init(conn_fd, conn_addr, m_root_path, m_connect_trig_mode, m_cgi);
            m_timer.add_sock(conn_fd);
        }
    }
}

void WebServer::deal_shutdown_connection(int sock_fd)
{
    LOG_INFO("Shutdown connection: %d", sock_fd);
    m_timer.del_sock(sock_fd);
    --HttpConnection::user_count;
}

void WebServer::deal_arrived_signal(int sock_fd) {
    int sig;
    char signals[1024] = {0};
    int ret = recv(sock_fd, signals, sizeof(signals), 0);

    if (ret <= 0)
        return ;
    else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
                case SIGALRM: {
                    LOG_INFO("Receive signal SIGALRM");
                    m_timer.tick_sock();
                    m_time_out = true;
                    break;
                }
                case SIGTERM: {
                    LOG_INFO("Receive signal SIGTERM");
                    m_stop_server = true;
                    break;
                }
                // default:
                    // LOG_WARNING("Unkown signal");
            }
        }
    }
}

void WebServer::deal_read_request(int sock_fd)
{
    LOG_INFO("Deal %d read request", sock_fd);
    m_users[sock_fd].m_state = 0;
    if (m_actor_mode == ActorMode::PROACTOR) {
        if (m_users[sock_fd].read()) {
            threadPool::singleton()->add_task(&m_users[sock_fd]);
        }
        else {
            m_timer.del_sock(sock_fd);
            --HttpConnection::user_count;
        }
    }
    else if (m_actor_mode == ActorMode::REACTOR) {
        threadPool::singleton()->add_task(&m_users[sock_fd]);

        //reactor由主线程管理IO增删改查
        while (!m_users[sock_fd].m_has_deal);

        m_users[sock_fd].m_has_deal = false;
      
        if (m_users[sock_fd].m_kill) {
            m_timer.del_sock(sock_fd);
            --HttpConnection::user_count;
        }
    }
}

void WebServer::deal_write_request(int sock_fd)
{
    LOG_INFO("Deal %d write request", sock_fd);
    m_users[sock_fd].m_state = 1;
    if (m_actor_mode == ActorMode::PROACTOR) {
        if (!m_users[sock_fd].write()) {
            m_timer.del_sock(sock_fd);
            --HttpConnection::user_count;
        }
    }
    else if (m_actor_mode == ActorMode::REACTOR) {
        threadPool::singleton()->add_task(&m_users[sock_fd]);
        while (!m_users[sock_fd].m_has_deal);
        m_users[sock_fd].m_has_deal = false;
        if (m_users[sock_fd].m_kill) {
            m_timer.del_sock(sock_fd);
            --HttpConnection::user_count;
        }
    }
}
