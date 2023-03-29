#ifndef SJ_COMMON_H
#define SJ_COMMON_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
// #include <bits/sigaction.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#define FILE_NAME_LEN 200
#define BUF_SIZE 2048
#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 1024
//最大连接数
#define MAX_CONNECTION 65536
#define MAX_EVENTS_NUMBER 10000
enum TrigMode {
    ET = 0,
    LT
};

enum ActorMode {
    PROACTOR = 0,
    REACTOR
};

class HttpState {
public:
    enum Method {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PACH
    };

    enum CheckState {
        //parse request line
        CHECK_STATE_REQUESTLINE = 0,
        //parse request header
        CHECK_STATE_HEADER,
        //parse request content
        CHECK_STATE_CONTENT,    
    };

    enum HttpCode {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LineState {
        //完整读取一行
        LINE_OK = 0,
        //报文语法有错误
        LINE_BAD,
        //读取的行不完整
        LINE_OPEN 
    };
};

class Utils {
public:
    static void epoll_add_fd(int epoll_fd, int fd, bool one_shot, TrigMode mode);
    static void epoll_remove_fd(int epoll_fd, int fd);
    static void epoll_motify_fd(int epoll_fd, int fd, int event, TrigMode mode);

    static int set_nonblocking(int fd);

    static void sig_handler(int sig);

    static void add_sig(int sig, void(handler)(int), bool restart);

public:
    static int *m_pipe_fd;
    static int m_epoll_fd;
    static float m_time_threshold;
};

#endif