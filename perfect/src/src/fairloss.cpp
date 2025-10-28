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

    bool send_k(char id, char k, unsigned long seq_nr, struct sockaddr_in* dest) {
      // char + char + 8 * long + null
      const char n = sizeof(unsigned long);
      char buffer[2 + 8 * n + 1] = {0};
      char* ptr = buffer;
      *ptr = static_cast<char>(id + '0'); // at most 128 so fine
      ptr++;
      *ptr = static_cast<char>(k + '0'); // at most 8 so fine
      ptr++;
      for (char i = 0; i < k; i++) {
        // write the longs as hexadecimals
        snprintf(ptr, 2*n, "%lx", seq_nr + i);
        while (*ptr != 0)
          ptr++;
        if (i < k - 1) {
          *ptr = 31; // ascii unit separator as msg separator
          ptr++;
        }
      }

      if (OO) {
        std::cout << "fl_s sending buffer '" << buffer << "'\n";
      }

      ssize_t bytes_sent = sendto(sock, buffer, strlen(buffer), 0,
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

    void receive(unsigned long* sender_id, char** buffer) {
      sockaddr_in from;
      socklen_t from_len = sizeof(from);
      ssize_t msg_len = recvfrom(sock, *buffer, 256, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
      if (msg_len < 0) {
        perror("reading error...\n");
        close(sock);
        exit(-1);
      }

      char* end = *buffer + msg_len;

      *sender_id = static_cast<char>(**buffer - '0');
      (*buffer)++;
      char k = static_cast<char>(**buffer - '0');
      (*buffer)++;
      
      if (OO)
        std::cout << "fl_r " << static_cast<int>(*sender_id)  << " " << static_cast<int>(k)  << " packets" << std::endl;
      
      std::cout << "fl_r buffer " << *buffer << std::endl;
      for (char i = 0; i < k; i++) {
        char* msg = *buffer;
        char* sep = std::find(msg, end, static_cast<char>(31));
        *sep = '\0';
        if (OO)
          std::cout << "fl_r " << static_cast<int>(*sender_id) << ": " << msg << std::endl;
        *buffer = sep + 1;
      }
      // if (**buffer != '\0') // safety
      //   (*buffer)++;
      // // printf("%.*s", 4, buff + 10);

      // std::cout << "fl_r " << *sender_id << " " << *buffer << std::endl;
    }

  private:
    int sock;
};