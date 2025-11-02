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

    void send(char* msg, struct sockaddr_in* dest) {
      st.send(msg, dest);
    }

    void run() {
      st.run();
    }

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void receive(std::function<void(unsigned char, char*)> callback, unsigned char sender, char* msg, char* end) {
      char* sep = msg;
      if (OO >= 1)
        std::cout << "buffer (size " << (end-msg) << ") " << msg << std::endl;
      while (sep != end) {
        char* sub_msg = sep;
        if (OO >= 1)
          std::cout << "rest buffer " << sub_msg << std::endl;
        sep = std::find(sep, end, static_cast<char>(31));
        // end sub_msg (instead of unit separator 31)
        if (end != sep)
          *sep = '\0';

        // receive
        if (OO >= 1)
          std::cout << "pf_r " << static_cast<int>(sender)
            << ": " << sub_msg << std::endl;
        // receive on sender
        callback(sender, sub_msg);

        if (end != sep)
          sep++;
        // if (OO >= 1)
        //   printf("end %p, sep %p, sub_msg %p", end, sep, sub_msg);
      }
    }

    void listen(std::function<void(unsigned char, char*)> callback) {
      auto fun = std::bind(&Perfect::receive, this, callback,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      st.listen(fun);
    }

  private:
    Stubborn st;
};