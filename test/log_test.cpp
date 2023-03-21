#include <thread>
#include <stdlib.h>
#include <unistd.h>

#include "../log/log.h"

void test_init() {
    const char* dir_name = "../record/";
    size_t max_line = 10000;
    size_t max_size = 1000;
    bool is_asyc = true;
    bool is_close = false;
    
    Log::singleton()->init(dir_name, max_line, max_size, true, is_close);
}

void write_debug_info() {
    while (true) {
        LOG_DEBUG("%s\n", "this is debug info");
        sleep(1);
    }
}

void write_info() {
    while (true) {
        LOG_INFO("%s\n", "this is info");
        sleep(1);
    }
}

int main() {
    test_init();

    thread t1(write_debug_info);
    thread t2(write_info);

    t1.join();
    t2.join();

    return 0;
}