#ifndef SJ_HTTP_CONNECTION_H
#define SJ_HTTP_CONNECTION_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <string>

#include <mysql/mysql.h>

#include "webServer/common/util.h"

class HttpConnection {
public:
    HttpConnection() {};
    ~HttpConnection() {};

    //http实例初始化
    void init(int sock_fd, bool cgi, std::string doc_root, TrigMode trig_mode, 
    std::string sql_name, std::string sql_usr, std::string sql_passwd);
    

public:
    static int epoll_fd;
    static int m_user_count;
    MYSQL *m_mysql;
    //读为0，写为1
    int m_state;

private:
    int m_sock_fd;
    sockaddr_in m_address;

    //存储请求报文数据
    char m_read_buf[HttpState::READ_BUFFER_SIZE];
    //缓冲区中第一个空闲的位置
    int m_read_idx;
    //缓冲区中第一个未读取的位置
    int m_read_idx;
    //缓冲区中已经解析的字符长度
    int m_parse_len;

    //存储响应报文数据
    char m_write_buf[HttpState::WRITE_BUFFER_SIZE];
    //缓冲区第一个空闲位置
    int m_write_idx;

    //主状态机状态
    HttpState::CheckState m_check_state;

    //请求方法
    HttpState::Method m_method;
    //请求文件的名称
    char m_request_file[HttpState::FILENAME_LEN];
    //请求的url
    char *m_url;
    //请求的协议版本
    char *m_version;
    //请求的主机名称
    char *m_host;
    //请求的内容长度
    int m_content_len;
    //优雅地关闭连接与否
    bool m_linger;

    //服务器上文件地址
    char *m_file_addr;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    //是否启用post
    bool m_cgi;
    //存储请求头数据
    char *m_string;
    //剩余需要发送的数据大小
    int m_bytes_to_send;
    //已经发送的数据大小
    int m_bytes_have_send;

    std::string m_doc_root;
    TrigMode m_trig_mode;

    std::string m_sql_user;
    std::string m_sql_passwd;
    std::string m_sql_name;
};

#endif