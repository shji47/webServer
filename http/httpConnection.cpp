#include "httpConnection.h"
#include "../mysqlPool/mysqlPool.h"
#include "../log/log.h"

int HttpConnection::epoll_fd = 0;
int HttpConnection::user_count = 0;
std::map<std::string, std::string> HttpConnection::usr_infos = std::map<std::string, std::string>();
std::map<std::string, std::string> HttpConnection::web_resource = std::map<std::string, std::string>();

void HttpConnection::init(int sock_fd, sockaddr_in address, std::string doc_root, TrigMode trig_mode, bool cgi) {
    init();
    
    m_sock_fd = sock_fd;
    m_address = std::move(address);
    m_doc_root = std::move(doc_root);
    m_trig_mode = trig_mode;
    m_cgi = cgi;

    Utils::epoll_add_fd(HttpConnection::epoll_fd, sock_fd, true, m_trig_mode);

    m_linger = false;
    m_content_len = 0;

    m_mysql = nullptr;
}

void HttpConnection::init() {
    m_free_idx = 0;
    m_read_idx = 0;
    m_parse_len = 0;
    m_write_idx = 0;

    m_check_state = HttpState::CheckState::CHECK_STATE_REQUESTLINE;
    m_request_method = HttpState::Method::GET;
    m_method = nullptr;
    m_url = nullptr;
    m_version = nullptr;
    m_host = nullptr;
    m_content_len = 0;
    m_linger = false;
    m_cgi = false;

    m_file_addr = nullptr;
    m_iv_count = 0;
    m_string = nullptr;
    m_bytes_to_send = 0;
    m_bytes_have_send = 0;

    m_has_deal = false;
    m_kill = false;

    m_mysql = nullptr;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    // memset(m_request_file, '\0', FILE_NAME_LEN);
}

HttpState::LineState HttpConnection::parse_line() {
    // std::cout<<"debug: parese data....."<<std::endl;
    for (; m_read_idx < m_free_idx; ++m_read_idx) {
        if (m_read_buf[m_read_idx] == '\r') {
            //缓冲区数据读完，但是一行数据没有读完
            if (m_read_idx + 1 >= m_free_idx)
                return HttpState::LineState::LINE_OPEN;
            //一行数据完整读完
            else if (m_read_buf[m_read_idx + 1] == '\n') {
                m_read_buf[m_read_idx++] = '\0';
                m_read_buf[m_read_idx++] = '\0';
                // std::cout<<"debug: parse data: "<<m_read_buf + m_parse_len<<std::endl;
                return HttpState::LineState::LINE_OK;
            }
            return HttpState::LineState::LINE_BAD;
        }
        else if (m_read_buf[m_read_idx] == '\n') {
            if (m_read_idx >= 1 && m_read_buf[m_read_idx - 1] == '\r') {
                m_read_buf[m_read_idx - 1] = '\0';
                m_read_buf[m_read_idx++] = '\0';
                // std::cout<<"debug: parse data: "<<m_read_buf + m_parse_len<<std::endl;
                return HttpState::LineState::LINE_OK;
            }
            return HttpState::LineState::LINE_BAD;
        }
    }

    return HttpState::LineState::LINE_OPEN;
}

//method url version
//example: GET https://baidu.com HTTP/1.1
HttpState::HttpCode HttpConnection::parse_requset_line(char* data) {
    m_method = data;
    m_url = strpbrk(data, " \t");
    if (!m_url)
        return HttpState::HttpCode::BAD_REQUEST;
    *m_url++ = '\0';

    if (strcmp(m_method, "GET") == 0) {
        m_request_method = HttpState::Method::GET;
    }
    else if (strcmp(m_method, "POST") == 0) {
        m_request_method = HttpState::Method::POST;
    }
    else {
        LOG_INFO("BAD REQUEST: %s is unkown method", m_method);
        return HttpState::HttpCode::BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");

    m_version = strpbrk(m_url, " \t");
    if (!m_version)
        return HttpState::HttpCode::BAD_REQUEST;
    
    *m_version++ = '\0';

    m_version += strspn(m_version, " \t");
    //当前仅支持HTTP1.1

    if (strcmp(m_version, "HTTP/1.1") != 0) {
        return HttpState::HttpCode::BAD_REQUEST;
    }

    // if (strncasecmp(m_url, "http://", 7) == 0) {
    //     m_url += 7;
    //     m_url = strchr(m_url, '/');
    // }
    // else if (strncasecmp(m_url, "https://", 8) == 0) {
    //     m_url += 8;
    //     m_url = strchr(m_url, '/');
    // }
    // else {
    //     return HttpState::HttpCode::BAD_REQUEST;
    // }

    if (!m_url || m_url[0] != '/')
        return HttpState::HttpCode::BAD_REQUEST;

    if (strlen(m_url) == 1)
        strcat(m_url, "hello.html");

    m_check_state = HttpState::CheckState::CHECK_STATE_HEADER;

    return HttpState::HttpCode::NO_REQUEST;
}

HttpState::HttpCode HttpConnection::parse_header(char *data) {
    if (data[0] == '\0') {
        if (m_content_len != 0) {
            m_check_state = HttpState::CheckState::CHECK_STATE_CONTENT;
            return HttpState::HttpCode::NO_REQUEST;
        }

        return HttpState::HttpCode::GET_REQUEST;
    }
    else if (strncasecmp(data, "Connection:", 11) == 0) {
        data += 11;
        data += strspn(data, " \t");
        if (strcasecmp(data, "keep_alive") == 0)
            m_linger = true;
    }
    else if (strncasecmp(data, "Context-length:", 15) == 0) {
        data += 15;
        data += strspn(data, " \t");
        m_content_len = atoi(data);
    }
    else if (strncasecmp(data, "Host:", 5) == 0) {
        data += 5;
        data += strspn(data, " \t");
        m_host = data;
    }
    else {
        LOG_INFO("The header segment which is not supported: %s", data);
    }

    return HttpState::HttpCode::NO_REQUEST;
}

HttpState::HttpCode HttpConnection::parse_content(char *data) {
    //所有请求报文数据都被读入缓冲区中
    if (m_free_idx >= m_read_idx + m_content_len) {
        data[m_content_len] = '\0';
        m_string = data;
        return HttpState::HttpCode::GET_REQUEST;
    }

    return HttpState::HttpCode::NO_REQUEST;
}

HttpState::HttpCode HttpConnection::deal_with_request() {
    char file_path[FILE_NAME_LEN];
    strcpy(file_path, m_doc_root.c_str());
    int len = m_doc_root.size();

    const char *p = strrchr(m_url, '/');
    
    if (m_cgi && (*(p + 1) == '2' || *(p + 1) == '3')) {
        char *url_path = (char*)malloc(sizeof(char) * FILE_NAME_LEN);
        //感觉没什么用
        strcpy(url_path, "/");
        strcpy(url_path, m_url+2);
        strncpy(file_path+len, url_path, FILE_NAME_LEN - len - 1);
        free(url_path);

        //如果是登陆或者注册，content中有用户名和密码信息
        //这里为了避免复制，对保存的content数据直接进行了修改
        //user=sj&password=1234
        char *usr_name;
        char *usr_passwd;
        //usr_name = "user=sj&password=1234"
        usr_name = m_string;
        //m_string = "&password=1234"
        m_string = strchr(m_string, '&');
        //usr_name = "user=sj"
        //m_string = "password=1234"
        *m_string++ = '\0';
        usr_name += 5;
        usr_passwd = m_string + 10;
        //注册请求
        if (*(p + 1) == '3') {
            //注册前检查用户名是否重复
            char *sql_insert = (char*)malloc(sizeof(char) * FILE_NAME_LEN);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, usr_name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, usr_passwd);
            strcat(sql_insert, "')");

            m_mutex.lock();
            if (usr_infos.find(usr_name) == usr_infos.end()) {
                m_mutex.unlock();
                //数据库应该是线程安全的
                //操作耗时，先看看不锁性能如何
                int res = mysql_query(m_mysql, sql_insert);
                m_mutex.lock();
                usr_infos.insert(std::pair<std::string, std::string>(usr_name, usr_passwd));
                m_mutex.unlock();

                if (!res) {
                    strcpy(m_url, web_resource["login"].c_str());
                }
                else
                    strcpy(m_url, web_resource["registerError"].c_str());
            }
            else {
                m_mutex.unlock();
                strcpy(m_url, web_resource["registerError"].c_str());
            }
            free(sql_insert);
        }
        else {
            if (usr_infos.find(usr_name) != usr_infos.end() && usr_infos[usr_name] == usr_passwd)
                strcpy(m_url, web_resource["home"].c_str());
            else 
                strcpy(m_url, web_resource["loginError"].c_str());
        }
    }

    //当前就用数字来表示
    //就有一个界面
    if (*(p + 1) == '0') {
        char *web_file_path = (char*)malloc(sizeof(char) * FILE_NAME_LEN);
        strcpy(web_file_path, web_resource["home"].c_str());
        strncpy(file_path + len, web_file_path, strlen(web_file_path));
        free(web_file_path);
    }
    else {
        strncpy(file_path + len, m_url, FILE_NAME_LEN - len - 1);
    }

    // std::cout<<"debug: file path: "<<file_path<<std::endl;

    if (stat(file_path, &m_file_stat) < 0)
        return HttpState::HttpCode::NO_RESOURCE;
    
    if (!(m_file_stat.st_mode & S_IROTH))
        return HttpState::HttpCode::FORBIDDEN_REQUEST;
    //判断该文件是不是目录
    if (S_ISDIR(m_file_stat.st_mode))
        return HttpState::HttpCode::BAD_REQUEST;
    
    int fd = open(file_path, O_RDONLY);
    m_file_addr = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return HttpState::HttpCode::FILE_REQUEST;
}

HttpState::HttpCode HttpConnection::process_read() {
    HttpState::LineState line_state = HttpState::LineState::LINE_OK;
    HttpState::HttpCode read_code = HttpState::HttpCode::NO_REQUEST;
    char *data = nullptr;
    LOG_INFO("Request info: %s", m_read_buf);

    while ((m_check_state == HttpState::CheckState::CHECK_STATE_CONTENT && line_state == HttpState::LineState::LINE_OK) ||
    (line_state = parse_line()) == HttpState::LineState::LINE_OK) {
        data = get_line();
        m_parse_len = m_read_idx;
        LOG_INFO("Parse info: %s", data);
        // std::cout<<"debug: data: "<<data<<std::endl;
        switch(m_check_state) {
            case HttpState::CheckState::CHECK_STATE_REQUESTLINE: {
                read_code = parse_requset_line(data);
                if (read_code == HttpState::HttpCode::BAD_REQUEST)
                    return HttpState::HttpCode::BAD_REQUEST;
                break;
            }
            case HttpState::CheckState::CHECK_STATE_HEADER: {
                read_code = parse_header(data);
                // std::cout<<"debug: check header code: "<<read_code<<std::endl;
                if (read_code == HttpState::HttpCode::BAD_REQUEST) {
                    // std::cout<<"debug: bad request"<<std::endl;
                    return HttpState::HttpCode::BAD_REQUEST;
                }
                else if (read_code == HttpState::HttpCode::GET_REQUEST) {
                    // std::cout<<"debug: do request"<<std::endl;
                    return deal_with_request();
                }
                break;
            }
            case HttpState::CheckState::CHECK_STATE_CONTENT: {
                read_code = parse_content(data);
                if (read_code == HttpState::HttpCode::BAD_REQUEST)
                    return HttpState::HttpCode::BAD_REQUEST;
                else if (read_code == HttpState::HttpCode::GET_REQUEST)
                    return deal_with_request();
                line_state = HttpState::LineState::LINE_OPEN;
                break;
            }
            default: {
                return HttpState::HttpCode::INTERNAL_ERROR;
            }
        }
    }

    return HttpState::HttpCode::NO_REQUEST;
}

bool HttpConnection::add_response(const char *format, ...) {
    if (m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }

    va_list vlst;
    va_start(vlst, format);

    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx - 1, format, vlst);
    
    va_end(vlst);
    if (len >= (WRITE_BUFFER_SIZE - m_write_idx - 1)) {
        return false;
    }

    m_write_idx += len;
    LOG_INFO("Response: %s", m_write_buf);

    return true;
}

bool HttpConnection::add_status_line(int status, const char *title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConnection::add_headers(int content_len) {
    return add_content_len(content_len) && add_linger() && add_blank_line();
}

bool HttpConnection::add_content_len(int content_len) {
    return add_response("Content-Length:%d\r\n", content_len);
}

bool HttpConnection::add_content_type() {
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool HttpConnection::add_linger() {
    return add_response("Connection:%s\r\n", m_linger ? "keep-alive" : "close");
}

bool HttpConnection::add_blank_line() {
    return add_response("%s", "\r\n");
}

bool HttpConnection::add_content(const char *content) {
    return add_response("%s", content);
}

bool HttpConnection::process_write(HttpState::HttpCode read_code) {
    switch (read_code) {
        case HttpState::HttpCode::INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return false;
            break;
        }
        case HttpState::HttpCode::BAD_REQUEST: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return false;
            break;
        }
        case HttpState::HttpCode::FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return false;
            break;
        }
        case HttpState::HttpCode::FILE_REQUEST: {
            // std::cout<<"debug: deal with read code 5"<<std::endl;
            add_status_line(200, ok_200_title);
            // std::cout<<"debug: add status line successful"<<std::endl;
            // std::cout<<"debug: file size: "<<m_file_stat.st_size<<std::endl;
            if (m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_addr;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                m_bytes_to_send = m_write_idx + m_file_stat.st_size;

                return true;
            }
            else {
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    return false;
                break;
            }
        }
        default: 
            return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[1].iov_len = m_write_idx;
    m_iv_count = 1;
    m_bytes_to_send = m_write_idx;

    return true;
}

void HttpConnection::process() {
    //处理请求
    HttpState::HttpCode read_code = process_read();
    // std::cout<<"debug: read code: "<<read_code<<std::endl;
    if (read_code == HttpState::HttpCode::NO_REQUEST) {
        Utils::epoll_motify_fd(epoll_fd, m_sock_fd, EPOLLIN, m_trig_mode);
        return ;
    }
    
    bool write_code = process_write(read_code);
    // std::cout<<"debug: write code: "<<write_code<<std::endl;
    
    if (!write_code) {
        LOG_INFO("Colse sock %d", m_sock_fd);
        Utils::epoll_remove_fd(epoll_fd, m_sock_fd);
        --user_count;
    }

    Utils::epoll_motify_fd(epoll_fd, m_sock_fd, EPOLLOUT, m_trig_mode);
}

bool HttpConnection::read() {
    if (m_free_idx >= READ_BUFFER_SIZE)
        return false;

    int bytes_read = 0;

    if (m_trig_mode == TrigMode::LT) {
        bytes_read = recv(m_sock_fd, m_read_buf, READ_BUFFER_SIZE - m_free_idx, 0);

        if (bytes_read <= 0)
            return false;
    }
    else {
        while (true) {
            bytes_read = recv(m_sock_fd, m_read_buf + m_free_idx, READ_BUFFER_SIZE - m_free_idx, 0);
            if (bytes_read == -1) {
                //errno为EAGIN，表明数据没有接收到
                //errno为EWOULDBLOCK时，表明数据已经读取完毕
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    break;
                return false;
            }
            else if (bytes_read == 0)
                return false;
            m_free_idx += bytes_read;
        }
    }

    return true;
}

bool HttpConnection::write() {
    int write_bytes = 0;
    if (m_bytes_to_send <= 0) {
        Utils::epoll_motify_fd(epoll_fd, m_sock_fd, EPOLLIN, m_trig_mode);
        init();
        return true;
    }

    while (true) {
        write_bytes = writev(m_sock_fd, m_iv, m_iv_count);
        if (write_bytes < 0) {
            if (errno == EAGAIN) {
                Utils::epoll_motify_fd(epoll_fd, m_sock_fd, EPOLLOUT, m_trig_mode);
                return true;
            }
            if (m_file_addr) {
                munmap(m_file_addr, m_file_stat.st_size);
                m_file_addr = nullptr;
            }

            return false;
        }

        m_bytes_have_send += write_bytes;
        m_bytes_to_send -= write_bytes;

        if (m_bytes_have_send >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_addr + (m_bytes_have_send - m_write_idx);
            m_iv[1].iov_len = m_bytes_to_send;
        }
        else {
            m_iv[0].iov_len -= m_bytes_have_send;
            m_iv[0].iov_base = m_write_buf + m_bytes_have_send;
        }

        if (m_bytes_to_send <= 0) {
            if (m_file_addr) {
                munmap(m_file_addr, m_file_stat.st_size);
                m_file_addr = nullptr;
            }

            Utils::epoll_motify_fd(epoll_fd, m_sock_fd, EPOLLIN, m_trig_mode);
            
            //如果是长连接，初始化HTTP对象
            if (m_linger) {
                init();
                return true;
            }
            else 
                return false;
        } 
    }
}