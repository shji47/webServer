#ifndef SJ_TIMER_H
#define SJ_TIMER_H

#include <list>
#include <map>
#include <mutex>
#include <time.h>

#include "../common/util.h"

//使用最原始的LRU模式
using namespace std;

struct timerNode {
public:    
    int sock_fd;
    unsigned int time_eplased;
    timerNode *next;
    timerNode *pre;
public:
    timerNode(): sock_fd(-1), time_eplased(0), next(nullptr), pre(nullptr){}
    timerNode(int _sock_fd, unsigned int _time_eplased, timerNode *_next, timerNode *_pre): sock_fd(_sock_fd), time_eplased(_time_eplased), next(_next), pre(_pre) {} 
};

class Timer {
public:
    Timer(){
        m_time_head->next = m_time_tail;
        m_time_tail->pre = m_time_head;
    }
    ~Timer(){}
    
    void init(unsigned int time_slot) {
        m_time_slot = time_slot;
    }
    void add_sock(int sock_fd);
    timerNode* del_sock(int sock_fd);
    void update_sock(int sock_fd);
    void tick_sock();

private:
    map<int, timerNode*> m_sock_map;
    timerNode* m_time_head = new timerNode();
    timerNode* m_time_tail = new timerNode();
    unsigned int m_time_slot;
    mutex m_mutex;
};

#endif