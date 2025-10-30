#pragma once

#include <iostream>
#include <functional>

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

    void receive(std::function<void(unsigned char, char*)> callback, unsigned char sender, char* msg, ssize_t msg_len) {
      char* end = msg + msg_len;
      char* sep = msg;
      if (OO)
        std::cout << "buffer (size " << msg_len << ") " << msg << std::endl;
      while (sep != end) {
        char* sub_msg = sep;
        if (OO)
          std::cout << "rest buffer " << sub_msg << std::endl;
        sep = std::find(sep, end, static_cast<char>(31));
        // end sub_msg (instead of unit separator 31)
        *sep = '\0';

        // receive
        if (OO)
          std::cout << "pf_r " << static_cast<int>(sender)
            << ": " << sub_msg << std::endl;
        // receive on sender
        callback(sender, sub_msg);

        if (end != sep)
          sep++;
      }

      // if (OO) std::cout << "st_r " << static_cast<short>(*sender_id) << " " << *buffer << std::endl;
      // std::cout << "pf_r " << static_cast<short>(sender) << " " << msg << std::endl;
    }

    void listen(std::function<void(unsigned char, char*)> callback) {
      auto fun = std::bind(&Perfect::receive, this, callback,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      st.listen(fun);
    }

  private:
    Stubborn st;
};