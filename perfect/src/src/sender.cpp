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
      const char* msg = std::to_string(i).c_str();
      if (network.send(msg, receiver))
        log(i+1);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void log(unsigned long seq_nr) {
    out << "b " << seq_nr << "\n";
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