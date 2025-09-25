#pragma once

#include <iostream>

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
    network.send("fairloss message\n", receiver);
  }

private:
  FairLoss network;
  sockaddr_in* receiver;
  unsigned long id;
  unsigned long m;

};