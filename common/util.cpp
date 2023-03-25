#include "util.h"
int *Utils::m_pipe_fd = nullptr;

void Utils::epoll_add_fd(int epoll_fd, int fd, bool one_shot, TrigMode mode) {
    epoll_event event;
    event.data.fd = fd;
    
    //EPOLLIN:事件会在对端有数据写入时触发
    //EPOLLRDHUP:事件会在对端链接关闭时触发
    event.events = EPOLLIN | EPOLLRDHUP;

    if (mode == TrigMode::ET)
        event.events |= EPOLLET;
    
    if (one_shot)
        event.events |= EPOLLONESHOT;
    
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
}

void Utils::epoll_remove_fd(int epoll_fd, int fd){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

void Utils::epoll_motify_fd(int epoll_fd, int fd, int event, TrigMode mode) {
    epoll_event events;
    events.data.fd = fd;

    if (mode == TrigMode::ET) {
        events.events = event | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    }
    else {
        events.events = event | EPOLLONESHOT | EPOLLRDHUP;
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &events);
}

int Utils::set_nonblocking(int fd)
{
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);

        return old_option;
}

void Utils::sig_handler(int sig) {
    //保证函数的可重入性，保存原来的errno
    int save_erron = errno;
    char *msg = (char*)&sig;
    send(m_pipe_fd[1], msg, 1, 0);
    errno = save_erron;
}

void Utils::add_sig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    //执行信号处理后自动重启动先前中断的系统调用
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}