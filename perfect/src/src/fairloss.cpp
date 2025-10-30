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

    void receive(unsigned char* sender_id, char** buffer) {
      sockaddr_in from;
      socklen_t from_len = sizeof(from);
      ssize_t msg_len = recvfrom(sock, *buffer, 256, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
      if (msg_len < 0) {
        perror("reading error...\n");
        close(sock);
        exit(-1);
      }

      char* end = *buffer + msg_len;

      *sender_id = static_cast<unsigned char>(**buffer - '0');
      (*buffer)++;
      char k = static_cast<char>(**buffer - '0');
      (*buffer)++;
      
      if (OO)
        std::cout << "fl_r " << static_cast<int>(*sender_id)  << " " << static_cast<int>(k)  << " packets" << std::endl;
      
      std::cout << "fl_r buffer " << *buffer << std::endl;
      for (char i = 0; i < k; i++) {
        std::cout << "fl_r buffer1 " << *buffer << std::endl;
        char* msg = *buffer;
        char* sep = std::find(msg, end, static_cast<char>(31));
        *sep = '\0';
        if (OO)
        std::cout << "fl_r " << static_cast<int>(*sender_id) << ": " << msg << std::endl;
        std::cout << "fl_r buffer2 " << *buffer << std::endl;
        if (end != sep)
        *buffer = sep + 1;
      }
      std::cout << "fl_r buffer3 " << *buffer << std::endl;
      // if (**buffer != '\0') // safety
      //   (*buffer)++;
      // // printf("%.*s", 4, buff + 10);

      // std::cout << "fl_r " << *sender_id << " " << *buffer << std::endl;
    }

  private:
    int sock;
};