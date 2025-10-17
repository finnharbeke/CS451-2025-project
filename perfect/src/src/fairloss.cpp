#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

class FairLoss {
  public:
    FairLoss() {
      if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("couldn't create socket");
        exit(-1);
      }
    }

    void send(const char* msg, sockaddr_in* dest) {
      std::cout << "sending\n";

      ssize_t bytes_sent = sendto(sock, msg, strlen(msg), 0,
        reinterpret_cast<sockaddr*>(dest), sizeof(*dest));

      if (bytes_sent < 0) {
        std::cout << "couldn't send, errno " << errno << ", " << strerror(errno) << std::endl;
      }
    }

    void bind_address(sockaddr_in* address) {
      if (bind(sock, reinterpret_cast<sockaddr*>(address), sizeof(*address)) < 0) {
        perror("couldn't bind socket of receiver\n");
        close(sock);
        exit(-1);
      }
    }

    void receive() {

      sockaddr_in from;
      socklen_t from_len = sizeof(from);

      char buff[1024] = {0};
      std::cout << "reading..." << std::endl;
      ssize_t msg_len = recvfrom(sock, buff, 1024, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
      if (msg_len < 0) {
        perror("reading error...\n");
        close(sock);
        exit(-1);
      }
  
      std::cout.write(buff, msg_len);
      std::cout << from.sin_addr.s_addr << " " << from.sin_port << std::endl;
    }

  private:
    int sock;
};