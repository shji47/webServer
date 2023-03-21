#include <iostream>
#include <thread>
#include <random>
#include "../log/blockQueue.hpp"

using namespace std;

blockQueue<string> bq(10);

string getString() {
    int len = (double)rand() / RAND_MAX * 100;
    string str;
    for (int i = 0; i < len; ++i) {
        str.push_back('a');
    }

    return str;
}

void pfunc() {
    while (true) {
        string str = getString();
        bq.push(str);
    }
}

void cfunc() {
    while (true) {
        bq.pop();
    }
}
    

int main() {
    thread p1(pfunc);
    thread p2(pfunc);
    thread p3(pfunc);
    thread c1(cfunc);

    p1.join();
    p2.join();
    p3.join();
    c1.join();

    return 0;
}