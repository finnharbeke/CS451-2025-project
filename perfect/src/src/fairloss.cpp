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

    bool send(const char* msg, sockaddr_in* dest) {
      std::cout << "sending\n";

      ssize_t bytes_sent = sendto(sock, msg, strlen(msg), 0,
        reinterpret_cast<sockaddr*>(dest), sizeof(*dest));

      if (bytes_sent < 0) {
        std::cout << "couldn't send, errno " << errno << ", " << strerror(errno) << std::endl;
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

    void receive(unsigned long* sender_id, char** buffer) {
      sockaddr_in from;
      socklen_t from_len = sizeof(from);
      ssize_t msg_len = recvfrom(sock, *buffer, 256, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
      if (msg_len < 0) {
        perror("reading error...\n");
        close(sock);
        exit(-1);
      }

      *sender_id = std::strtoul(*buffer, buffer, 10);
      if (**buffer != '\0') // safety
        (*buffer)++;
      // printf("%.*s", 4, buff + 10);

      std::cout << "flr " << *sender_id << " " << *buffer << std::endl;
    }

  private:
    int sock;
};