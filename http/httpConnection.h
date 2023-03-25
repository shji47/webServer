#ifndef SJ_HTTP_CONNECTION_H
#define SJ_HTTP_CONNECTION_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <error.h>
#include <string.h>
#include <string>
#include <map>
#include <set>
#include <mutex>

#include <mysql/mysql.h>

#include "../common/util.h"

class HttpConnection {
private:
    const char *ok_200_title = "OK";
    const char *error_400_title = "Bad Request";
    const char *error_400_form = "Your request has bad syntax or is inherently impossible to stasify.\n";
    const char *error_403_title = "Forbidden";
    const char *error_403_form = "You don't have permission to get file from this server.\n";
    const char *error_404_title = "Not Found";
    const char *error_404_form = "The requested file was not found on this server.\n";
    const char *error_500_title = "Internal Error";
    const char *error_500_form = "There was an unusual problem serving the request file.\n";

public:
    HttpConnection() {};
    ~HttpConnection() {};

    //http实例初始化
    void init(int sock_fd, sockaddr_in address, std::string doc_root, TrigMode trig_mode, bool cgi);
    //实例开始处理与远端的读写
    void process();

    //对外读写接口
    bool read();
    bool write();

private:
    HttpState::HttpCode process_read();
    bool process_write(HttpState::HttpCode read_code);
    void init();

    //读
    char* get_line() {return m_read_buf + m_parse_len;}
    HttpState::LineState parse_line();
    HttpState::HttpCode parse_requset_line(char* data);
    HttpState::HttpCode parse_header(char* data);
    HttpState::HttpCode parse_content(char* data);
    HttpState::HttpCode deal_with_request();

    //写
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_len);
    bool add_content_type();
    bool add_content_len(int content_len);
    bool add_linger();
    bool add_blank_line();

public:
    static int epoll_fd;
    static int user_count;
    static std::map<std::string, std::string> usr_infos;
    static std::map<std::string, std::string> web_resource;
    //读为0，写为1
    int m_state;
    MYSQL *m_mysql;
    bool m_has_deal;
    bool m_kill;

private:
    int m_sock_fd;
    sockaddr_in m_address;

    //存储请求报文数据
    char m_read_buf[READ_BUFFER_SIZE];
    //缓冲区中第一个空闲的位置
    int m_free_idx;
    //缓冲区中第一个未读取的位置
    int m_read_idx;
    //缓冲区中解析的字符长度
    int m_parse_len;

    //存储响应报文数据
    char m_write_buf[WRITE_BUFFER_SIZE];
    //缓冲区第一个空闲位置
    int m_write_idx;

    //主状态机状态
    HttpState::CheckState m_check_state;

    //请求方法
    HttpState::Method m_request_method;
    // //请求文件的名称
    // char m_request_file[FILE_NAME_LEN];
    //请求的方法
    char *m_method;
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
    //开启cgi
    bool m_cgi;

    //服务器上文件地址
    char *m_file_addr;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    //存储post中content的数据
    char *m_string;
    //剩余需要发送的数据大小
    int m_bytes_to_send;
    //已经发送的数据大小
    int m_bytes_have_send;

    std::mutex m_mutex;

    std::string m_doc_root;
    TrigMode m_trig_mode;
};

#endif