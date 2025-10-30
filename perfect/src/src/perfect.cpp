#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "stubborn.cpp"
#include "log_queue.cpp"

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

    void receive(unsigned char sender, char* msg, ssize_t msg_len) {
      char* end = msg + msg_len;
      char* sep = msg;
      while (sep != end) {
        char* sub_msg = sep;
        char* sep = std::find(msg, end, static_cast<char>(31));
        *sep = '\0';
        if (OO)
          std::cout << "pf_r " << static_cast<int>(sender) << ": " << sub_msg << std::endl;
        if (end != sep)
          sep++;
      }

      // if (OO) std::cout << "st_r " << static_cast<short>(*sender_id) << " " << *buffer << std::endl;
      // std::cout << "pf_r " << static_cast<short>(sender) << " " << msg << std::endl;
    }

    void listen() {

    }

    void listen(std::function<void(ReceiveLog*)> callback) {
      ReceiveLog l;

    }

  private:
    Stubborn st;
};