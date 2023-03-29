#include "timer.h"

void Timer::add_sock(int sock_fd) {
    if (m_sock_map.count(sock_fd)) {
        update_sock(sock_fd);
        return ;
    }
    timerNode *node = new timerNode(sock_fd, time(0), nullptr, nullptr);
    timerNode *pre = m_time_tail->pre;
    pre->next = node;
    node->pre = pre;
    node->next = m_time_tail;
    m_time_tail->pre = node;
    m_sock_map.insert({sock_fd, node});
}

timerNode* Timer::del_sock(int sock_fd) {
    timerNode *node = m_sock_map[sock_fd];
    m_sock_map.erase(sock_fd);
    timerNode *next = node->next;
    timerNode *pre = node->pre;
    pre->next = next;
    next->pre = pre;
    node->next = nullptr;
    node->pre = nullptr;
    delete node;
    
    Utils::epoll_remove_fd(Utils::m_epoll_fd, sock_fd);
    return next;
}

void Timer::update_sock(int sock_fd) {
    timerNode *node = m_sock_map[sock_fd];
    timerNode *next = node->next;
    timerNode *pre = node->pre;
    pre->next = next;
    next->pre = pre;
    timerNode *head_next = m_time_head->next;
    node->next = head_next;
    head_next->pre = node;
    m_time_head->next = node;
    node->pre = m_time_head;
    node->time_eplased = time(0);
}

void Timer::tick_sock() {
    timerNode *cur = m_time_head->next;
    unsigned int now_time = time(0);
    while (cur != m_time_tail) {
        if (now_time - cur->time_eplased >= m_time_slot) {
            cur = del_sock(cur->sock_fd);
        }
        else {
            break;
        }
    }
}