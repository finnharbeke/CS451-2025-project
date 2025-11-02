#pragma once

#include <iostream>
#include <functional>
#include <unordered_set>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "fairloss.cpp"
#include "stubborn_heap.cpp"

class Stubborn {
  public:
    Stubborn() {}

    void send(char* msg, struct sockaddr_in* dest) {
      StubbornMsg stbmsg(counter, msg, dest);
      counter++;
      Q.add(stbmsg);
    }

    void run() {
      if (OO >= 3) std::cout << "running stubborn" << std::endl;
      while (true) {
        StubbornMsg stbmsg(0, nullptr, nullptr);
        if (OO >= 3) std::cout << "run loop" << std::endl;
        if (Q.next(&stbmsg)) {
          fl.send(stbmsg.msg, stbmsg.dest);
          // exponential backoff
          stbmsg.back_off();
          Q.add(stbmsg);
        }
      }
    }

    void bind_address(sockaddr_in* address) {
      fl.bind_address(address);
    }

    void receive(std::function<void(unsigned char, char*, char*)> callback, ssize_t msg_len, char* buffer) {

      char* end = buffer + msg_len;

      unsigned char sender = static_cast<unsigned char>(*buffer - '0');
      char* msg = nullptr;
      unsigned int msg_id = static_cast<unsigned int>(strtoul(buffer + 1, &msg, 16));
      if (OO >= 1)
        std::cout << "st_r " << static_cast<short>(sender)  << " " << msg_id << std::endl;
      msg++;
      auto hash = (static_cast<unsigned long>(sender) << 32) | msg_id;
      if (lookup.find(hash) != lookup.end()) {

      } else {
        lookup.emplace(hash);
        callback(sender, msg, end);
      }
    }

    void listen(std::function<void(unsigned char, char*, char*)> callback) {
      auto fun = std::bind(&Stubborn::receive, this, callback,
        std::placeholders::_1, std::placeholders::_2);
      fl.listen(fun);
    }

  private:
    FairLoss fl;
    StubbornHeap Q;
    std::unordered_set<unsigned long> lookup{};
    unsigned int counter = 0;
};