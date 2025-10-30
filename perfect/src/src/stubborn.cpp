#pragma once

#include <iostream>
#include <functional>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "fairloss.cpp"

class Stubborn {
  public:
    Stubborn() {}

    bool send(char* msg, struct sockaddr_in* dest) {
      // while (true) {
        fl.send(msg, dest);
      // }
      return true;
    }

    void bind_address(sockaddr_in* address) {
      fl.bind_address(address);
    }

    void receive(std::function<void(unsigned char, char*, ssize_t)> callback, ssize_t msg_len, char* buffer) {

      char* end = buffer + msg_len;

      unsigned char sender = static_cast<unsigned char>(*buffer - '0');
      char* msg = nullptr;
      unsigned int msg_id = static_cast<unsigned int>(strtoul(buffer + 1, &msg, 16));
      if (OO)
        std::cout << "st_r " << static_cast<short>(sender)  << " " << msg_id << std::endl;
      msg++;
      // TODO dupe checking
      callback(sender, msg, msg_len);
    }

    void listen(std::function<void(unsigned char, char*, ssize_t)> callback) {
      auto fun = std::bind(&Stubborn::receive, this, callback,
        std::placeholders::_1, std::placeholders::_2);
      fl.listen(fun);
    }

  private:
    FairLoss fl;
};