#ifndef SJ_COMMON_H
#define SJ_COMMON_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <bits/sigaction.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

enum TrigMode {
    ET = 0,
    LT
};

class HttpState {
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    const char *ok_200_title = "OK";
    const char *error_400_title = "Bad Request";
    const char *error_400_form = "Your request has bad syntax or is inherently impossible to stasify.\n";
    const char *error_403_title = "Forbidden";
    const char *error_403_form = "You don't have permission to get file from this server.\n";
    const char *error_404_title = "Not Found";
    const char *error_404_form = "The requested file was not found on this server.\n";
    const char *error_500_title = "Internal Error";
    const char *error_500_form = "There was an unusual problem serving the request file.\n";

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
    void epoll_add_fd(int epoll_fd, int fd, bool one_shot, TrigMode mode);
    void epoll_remove_fd(int epoll_fd, int fd);

    int set_nonblocking(int fd);

    static void sig_handler(int sig);

    void add_sig(int sig, void(handler)(int), bool restart);

public:
    static int *m_pipe_fd;
    static int m_epoll_fd;
};

#endif