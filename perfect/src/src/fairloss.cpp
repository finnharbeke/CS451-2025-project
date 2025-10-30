#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"

class FairLoss {
  public:
    FairLoss() {
      if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("couldn't create socket");
        exit(-1);
      }
    }

    bool send(char* msg, struct sockaddr_in* dest) {

      if (OO) {
        std::cout << "fl_s sending buffer '" << msg << "'\n";
      }

      ssize_t bytes_sent = sendto(sock, msg, strlen(msg), 0,
        reinterpret_cast<sockaddr*>(dest), sizeof(*dest));

      if (bytes_sent < 0) {
        std::cout << "fl_s couldn't send, errno " << errno << ", " << strerror(errno) << std::endl;
        return false;
      }
      return true;
    }

    void bind_address(sockaddr_in* address) {
      if (bind(sock, reinterpret_cast<sockaddr*>(address), sizeof(*address)) < 0) {
        perror("couldn't bind socket of receiver\n");
        close(sock);
        exit(-1);
      }
    }

    void listen(std::function<void(ssize_t, char*)> callback) {
      sockaddr_in from;
      socklen_t from_len = sizeof(from);
      char* buffer = static_cast<char*>(malloc(1024));
      ssize_t msg_len;

      while (true) {
        msg_len = recvfrom(sock, buffer, 1024, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (msg_len < 0) {
          perror("reading error...\n");
          // close(sock);
          // exit(-1);
        }
        if (OO) std::cout << "fl_r " << buffer << std::endl;
        callback(msg_len, buffer);
      }
    }

  private:
    int sock;
};