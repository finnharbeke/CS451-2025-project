#pragma once

#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

#include "global.h"

struct Log {
    bool send;
    unsigned char from; // sender
    unsigned int seq_nr;
    Log() {} // empty
    Log(unsigned int seq_nr) : send(true), seq_nr(seq_nr) {}
    Log(unsigned char sender, unsigned int seq_nr) : send(false), from(sender), seq_nr(seq_nr) {}
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
        std::cout << n_logged - last_n_logged << "," << Q.size() << ",";
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
            n_logged += popnlog();
        }
    }

    void start_logging() {
        std::thread logging(&LogQueue::keep_logging, this);
        logging.detach();
    }

    void log(Log& log) {
        if (log.send) {
            if (OO >= 3) std::cout << "logging " << static_cast<short>(log.seq_nr) << std::endl;
            buffer += "b ";
            buffer += std::to_string(log.seq_nr);
            buffer += "\n";
        } else {
            if (OO >= 3) std::cout << "logging " << log.seq_nr << " from " << static_cast<short>(log.from) << std::endl;
            buffer += "d ";
            buffer += std::to_string(static_cast<short>(log.from));
            buffer += " ";
            buffer += std::to_string(log.seq_nr);
            buffer += "\n";
        }
        if (buffer.size() >= LOGBUFSIZE - LOGLINESIZE) {
            out << buffer;
            buffer.clear();
        }
    }
    
    void close() {
        out << buffer;
        out.flush();
        out.close();
    }

    private:
        std::queue<Log> Q;
        std::mutex mutx;
        std::string buffer;
        std::ofstream out;
        unsigned int n_logged = 0;
        
        unsigned int last_n_logged = 0;
};