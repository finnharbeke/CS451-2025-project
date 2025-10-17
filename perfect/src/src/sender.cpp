#pragma once

#include <iostream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "fairloss.cpp"

class Sender {
public:
  Sender(unsigned long id, unsigned long m, sockaddr_in* receiver)
    : receiver(receiver), id(id), m(m) {
    std::cout << "setting up sender with process id " << id << std::endl;
  }

  void main() {
    for (unsigned long i = 0; i < m; i++) {
      network.send("fairloss message\n", receiver);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

private:
  FairLoss network;
  sockaddr_in* receiver;
  unsigned long id;
  unsigned long m;

};