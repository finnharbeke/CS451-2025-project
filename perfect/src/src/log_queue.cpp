#pragma once

#include <fstream>

#include <queue>
#include <thread>
#include <mutex>

struct SendLog {
    unsigned int seq_nr;
    SendLog(unsigned int seq_nr) : seq_nr(seq_nr) {}
};

struct ReceiveLog {
    unsigned int sender;
    char* msg;
    ReceiveLog(unsigned int sender, char* msg) : sender(sender), msg(msg) {}
};

// void log(*std::ofstream outfile, *SendLog log);
// void log(*std::ofstream outfile, *SendLog log);
// void log(*std::ofstream outfile, *SendLog log);

// help from
// https://www.geeksforgeeks.org/dsa/implement-thread-safe-queue-in-c/

template <typename T> class LogQueue {
public:
    void push(T t) {
        std::unique_lock<std::mutex> lock(mutx);
        Q.push(t);
    }

    bool pop(T* t) {
        std::unique_lock<std::mutex> lock(mutx);
        if (Q.empty())
            return false;

        if (OO) std::cout << "trying to get front" << std::endl;
        *t = Q.front();
        if (OO) std::cout << "got front" << std::endl;
        Q.pop();
        if (OO) std::cout << "popped" << std::endl;
        return true;
    }

    static void log(std::ofstream* outfile, SendLog* log) {
        std::cout << "logging " << log->seq_nr << std::endl;
        // (*outfile).write("b ");
        // (*outfile).write(log->seq_nr);
        // (*outfile).write("\n");
        (*outfile) << "b ";
        (*outfile) << log->seq_nr;
        (*outfile) << "\n";
    }

    static void log(std::ofstream* outfile, ReceiveLog* log) {
        *outfile << "d " << log->sender << " " << log->msg << "\n";
    }

    private:
        std::queue<T> Q;
        std::mutex mutx;
};