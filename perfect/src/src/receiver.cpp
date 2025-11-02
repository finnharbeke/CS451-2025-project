#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <functional>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "perfect.cpp"
#include "log_queue.cpp"

class Receiver {
public:
  
  Receiver(unsigned long id_, const char* outputPath,
      sockaddr_in* address) : address(address) {
    if (OO >= 1) std::cout << "setting up receiver with process id " << id_ << std::endl;
    
    id = static_cast<char>(id_ + '0'); // by assumptions at most 128
    out.open(outputPath);
    network.bind_address(address);

    if (OO >= 1) std::cout << "set up receiver " << id << std::endl;
  }

  void main() {
    std::thread listen(&Receiver::listen, this);
    listen.detach();
    std::thread logging(&Receiver::keep_logging, this);
    logging.detach();

  }

  void close() {
    if (OO >= 1) std::cout << "closing" << std::endl;
    out.close();
  }

  void receive(unsigned char sender, char* msg) {
    ReceiveLog log(sender, msg);
    queue.push(&log);
  }
  
  void listen() {
    auto fun = std::bind(&Receiver::receive, this,
      std::placeholders::_1, std::placeholders::_2);
    network.listen(fun);
  }
  
  void keep_logging() {
    while (true) {
      ReceiveLog log;
      
      if (queue.pop(&log)) {
        LogQueue<ReceiveLog>::log(&out, &log);
      }
    }
  }

  void log(unsigned long sender, char* msg) {
    out << "d " << sender << " " << msg << "\n";
    if (OOTIME) {
      n_logged++;
      if (n_logged % MSGS_PER_TIME == 0) {
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time (sec) for " << MSGS_PER_TIME << " messages = " << 
          static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) /1000000.0  << std::endl;
        begin = end;
      }
    }
  }

private:
  Perfect network;
  sockaddr_in* address;
  char id;
  char* outputPath;
  std::ofstream out;
  LogQueue<ReceiveLog> queue;
  const unsigned int MSGS_PER_TIME = 100000;
  unsigned int n_logged = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
};