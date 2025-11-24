#pragma once

#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

#include "global.h"

struct Log {
    bool send;
    unsigned int n; // seq_nr if send log, sender otherwise
    char* msg = nullptr;
    Log() : send(true), n(0) {} // empty
    Log(unsigned int seq_nr) : send(true), n(seq_nr) {}
    Log(unsigned char sender, char* msg) : send(false), n(sender), msg(msg) {}
};

// help from
// https://www.geeksforgeeks.org/dsa/implement-thread-safe-queue-in-c/

class LogQueue {
public:
    LogQueue(const char* outputPath) {
        out.open(outputPath);
        buffer.reserve(LOGBUFSIZE);
    }

    void stats() {
        std::cout << n_logged - last_n_logged << ",";
        last_n_logged = n_logged;
    }

    size_t size() {
        // now thread-safe apparently does matter
        std::lock_guard<std::mutex> lock(mutx);
        return Q.size();
    }

    void push(Log* l) {
        std::lock_guard<std::mutex> lock(mutx);
        Q.push(*l);
    }

    bool popnlog() {
        std::lock_guard<std::mutex> lock(mutx);
        if (Q.empty())
            return false;

        Log l = Q.front();
        Q.pop();
        log(l);
        return true;
    }

    void keep_logging() {
        while (true) {
            if (OO >= 3) std::cout << "log loop" << std::endl;
            n_logged += popnlog();
        }
    }

    void start_logging() {
        std::thread logging(&LogQueue::keep_logging, this);
        logging.detach();
    }

    void log(Log& log) {
        if (log.send) {
            if (OO >= 2) std::cout << "logging " << log.n << std::endl;
            buffer += "b ";
            buffer += std::to_string(log.n);
            buffer += "\n";
        } else {
            unsigned int seq_nr = static_cast<unsigned int>(strtoul(log.msg, nullptr, 16));
            buffer += "d ";
            buffer += std::to_string(log.n);
            buffer += " ";
            buffer += std::to_string(seq_nr);
            buffer += "\n";
        }
        if (buffer.size() >= LOGBUFSIZE - LOGLINESIZE) {
            out << buffer;
            buffer.clear();
        }
    }
    
    void close() {
        out << buffer;
        out.close();
    }

    private:
        std::queue<Log> Q;
        std::mutex mutx;
        std::string buffer;
        std::ofstream out;
        unsigned int n_logged = 0;
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        
        unsigned int last_n_logged = 0;
};