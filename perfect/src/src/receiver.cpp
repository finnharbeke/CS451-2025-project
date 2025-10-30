#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <functional>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "perfect.cpp"
#include "log_queue.cpp"

class Receiver {
public:
  
  Receiver(unsigned long id_, const char* outputPath,
      sockaddr_in* address) : address(address) {
    if (OO) std::cout << "setting up receiver with process id " << id << std::endl;
    
    id = static_cast<char>(id_ + '0'); // by assumptions at most 128
    out.open(outputPath);
    network.bind_address(address);

    if (OO) std::cout << "set up receiver " << id << std::endl;
  }

  void main() {
    std::thread listen(&Receiver::keep_listening, this);
    listen.detach();
    std::thread logging(&Receiver::keep_logging, this);
    logging.detach();

  }

  void close() {
    std::cout << "closing" << std::endl;
    out.close();
  }

  void receive(ReceiveLog* log) {
    queue.push(log);
  }
  
  void keep_listening() {
    auto fun = std::bind(&Receiver::receive, this, std::placeholders::_1);
    network.listen(fun);
  }
  
  void keep_logging() {

  }

  void log(unsigned long sender, char* msg) {
    out << "d " << sender << " " << msg << "\n";
  }

private:
  Perfect network;
  sockaddr_in* address;
  char id;
  char* outputPath;
  std::ofstream out;
  LogQueue<ReceiveLog> queue;
};