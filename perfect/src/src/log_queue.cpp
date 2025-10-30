#pragma once

#include <fstream>

#include <queue>
#include <thread>
#include <mutex>

struct SendLog {
    unsigned int seq_nr;
    SendLog(unsigned int seq_nr) : seq_nr(seq_nr) {}
    SendLog() : seq_nr(0) {}
};

struct ReceiveLog {
    unsigned int sender;
    char* msg;
    ReceiveLog(unsigned int sender, char* msg) : sender(sender), msg(msg) {}
    ReceiveLog() : sender(0), msg(nullptr) {}
};

// void log(*std::ofstream outfile, *SendLog log);
// void log(*std::ofstream outfile, *SendLog log);
// void log(*std::ofstream outfile, *SendLog log);

// help from
// https://www.geeksforgeeks.org/dsa/implement-thread-safe-queue-in-c/

template <typename T> class LogQueue {
public:
    void push(T* t) {
        std::unique_lock<std::mutex> lock(mutx);
        Q.push(*t);
    }

    bool pop(T* t) {
        std::unique_lock<std::mutex> lock(mutx);
        if (Q.empty())
            return false;

        *t = Q.front();
        Q.pop();
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
        unsigned int seq_nr = static_cast<unsigned int>(strtoul(log->msg, nullptr, 16));
        (*outfile) << "d " << log->sender << " " << seq_nr << "\n";
    }

    private:
        std::queue<T> Q;
        std::mutex mutx;
};