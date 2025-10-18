#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "fairloss.cpp"

class Stubborn {
  public:
    Stubborn() {}

    bool send(const char* msg, sockaddr_in* dest) {
      while (true) {
        fl.send(msg, dest);
      }
    }

    void bind_address(sockaddr_in* address) {
      fl.bind_address(address);
    }

    void receive(unsigned long* sender_id, char** buffer) {
      fl.receive(sender_id, buffer);

      std::cout << "slr " << *sender_id << " " << *buffer << std::endl;
    }

  private:
    FairLoss fl;
};