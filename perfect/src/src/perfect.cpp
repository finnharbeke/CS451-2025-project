#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "stubborn.cpp"

class Perfect {
  public:
    Perfect() {}

    bool send_k(char id, char k, unsigned long seq_nr, struct sockaddr_in* dest) {
      st.send_k(id, k, seq_nr, dest);
      return true;
    }

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void receive(unsigned long* sender_id, char** buffer) {
      st.receive(sender_id, buffer);

      std::cout << "pf_r " << *sender_id << " " << *buffer << std::endl;
    }

  private:
    Stubborn st;
};