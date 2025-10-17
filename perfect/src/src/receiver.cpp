#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "fairloss.cpp"

class Receiver {
public:
  
  Receiver(unsigned long id, unsigned long m, sockaddr_in* address, std::vector<sockaddr_in>* senders)
    : address(address), senders(senders) {
    std::cout << "setting up receiver with process id " << id << std::endl;
    network.bind_address(address);
  }

  void main() {
    while (true) {
      network.receive();
    }
  }

private:
  FairLoss network;
  sockaddr_in* address;
  std::vector<sockaddr_in>* senders;

};