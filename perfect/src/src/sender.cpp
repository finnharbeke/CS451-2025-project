#pragma once

#include <iostream>
#include <fstream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "fairloss.cpp"

class Sender {
public:
  Sender(unsigned long id, unsigned long m, const char* outputPath, sockaddr_in* receiver)
    : receiver(receiver), id(id), m(m) {
    std::cout << "setting up sender with process id " << id << std::endl;
    out.open(outputPath);
  }

  void main() {
    for (unsigned long i = 0; i < m; i++) {
      send(i+1);
    }
  }
  
  void send(unsigned long msg) {
    char buffer[256];
    snprintf(buffer, 256, "%lu %lu", id, msg);
    if (network.send(buffer, receiver)) {
      std::cout << "sent " << id << " " << msg << std::endl;
      log(msg);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  void log(unsigned long msg) {
    out << "b " << msg << "\n";
  }

  void close() {
    out.close();
  }

private:
  FairLoss network;
  sockaddr_in* receiver;
  unsigned long id;
  unsigned long m;
  std::ofstream out;
};