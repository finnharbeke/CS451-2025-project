#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "fairloss.cpp"

class Stubborn {
  public:
    Stubborn() {}

    bool send(char* msg, struct sockaddr_in* dest) {
      // while (true) {
        // fl.send_k(id, k, seq_nr, dest);
      // }
      return true;
    }

    void bind_address(sockaddr_in* address) {
      fl.bind_address(address);
    }

    void receive(unsigned char* sender_id, char** buffer) {
      fl.receive(sender_id, buffer);

      if (OO) std::cout << "st_r " << static_cast<short>(*sender_id) << " " << *buffer << std::endl;
    }

  private:
    FairLoss fl;
};