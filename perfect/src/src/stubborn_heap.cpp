#pragma once

#include <fstream>

#include <queue>
#include <thread>
#include <mutex>

struct StubbornMsg {
    unsigned int timeout;
    char* msg;
    struct sockaddr_in* dest;
    
    StubbornMsg(char* msg, struct sockaddr_in* dest)
        : timeout(1), msg(msg), dest(dest) {}
    StubbornMsg(unsigned int timeout, char* msg, struct sockaddr_in* dest)
        : timeout(timeout), msg(msg), dest(dest) {}
};

class StubbornHeap {
public:
    void add(StubbornMsg* msg) {
        std::unique_lock<std::mutex> lock(mutx);
        Q.push(*msg);
    }

    bool next(StubbornMsg* msg) {
        std::unique_lock<std::mutex> lock(mutx);
        if (Q.empty())
            return false;

        *msg = Q.top();
        Q.pop();
        return true;
    }

    

    private:
        std::priority_queue<StubbornMsg> Q;
        std::mutex mutx;
};