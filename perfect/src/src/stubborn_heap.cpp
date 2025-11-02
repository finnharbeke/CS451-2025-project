#pragma once

#include <fstream>

#include <queue>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>

struct StubbornMsg {
    unsigned int t0;
    unsigned int timeout;
    char* msg;
    struct sockaddr_in* dest;
    
    StubbornMsg(unsigned int t0, char* msg, struct sockaddr_in* dest)
        : t0(t0), timeout(t0), msg(msg), dest(dest) {}
    
    void back_off() {
        // this->timeout *= 2;
    }

    bool operator<(const StubbornMsg& b) const {
        return t0 < b.t0;
        // return false;
    }
};

// bool StubbornMsgCmp (StubbornMsg& a, StubbornMsg& b);
// bool StubbornMsgCmp (StubbornMsg& a, StubbornMsg& b) {
//     // unsigned int ta = a.t0 + a.timeout;
//     // unsigned int tb = b.t0 + b.timeout;
//     // return ta > tb;
//     return a.t0 > b.t0;
//     // return static_cast<signed long>(a.t0) > static_cast<signed long>(b.t0);
//     // return false;
// }


class StubbornHeap {
public:
    void add(StubbornMsg& msg) {
        std::unique_lock<std::mutex> lock(mutx);
        // https://stackoverflow.com/questions/63405519/why-does-stdpush-heap-generate-a-wstrict-overflow-3-warning-even-if-no-signed
        // we shouldn't use -Werror with -Wstrict-overflow
        // #pragma GCC diagnostic push
        // #pragma GCC diagnostic ignored "-Werror=strict-overflow"
        // std::vector<unsigned int> v;
        // v.push_back(3);
        // std::push_heap(v.begin(), v.end());
        Q.push(msg);
        // #pragma GCC diagnostic pop
    }

    bool next(StubbornMsg* msg) {
        std::unique_lock<std::mutex> lock(mutx);
        if (Q.empty())
            return false;

        // *msg = Q.top();
        *msg = Q.front();
        Q.pop();
        return true;
    }

    private:
        // std::priority_queue<StubbornMsg, std::vector<StubbornMsg>, StubbornMsgCmp> Q;
        // std::vector<StubbornMsg> heap;
        // std::priority_queue<StubbornMsg> Q;
        std::queue<StubbornMsg> Q;
        std::mutex mutx;
};