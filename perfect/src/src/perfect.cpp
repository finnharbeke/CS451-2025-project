#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "stubborn.cpp"

class Perfect {
  public:
    Perfect() {}

    bool send(char* msg, struct sockaddr_in* dest) {
      st.send(msg, dest);
      return true;
    }

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void receive(unsigned char* sender_id, char** buffer) {
      st.receive(sender_id, buffer);

      std::cout << "pf_r " << static_cast<short>(*sender_id) << " " << *buffer << std::endl;
    }

  private:
    Stubborn st;
};