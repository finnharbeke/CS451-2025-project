#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "readerwriterqueue.h"

class FairLoss {
  public:
    FairLoss(std::function<void(char*, ssize_t)> app_receive) :
      msg_queue(moodycamel::ReaderWriterQueue<std::pair<char*, ssize_t>>(MSGQSIZE)),
      app_receive(app_receive) {
      if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("couldn't create socket");
        exit(-1);
      }
      // setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &RCVBUFSIZE, sizeof(RCVBUFSIZE));
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << "," << recv_in_q - last_recv_in_q << std::endl;
      last_sent = sent;
      last_recv = recv;
      last_recv_in_q = recv_in_q;
    }

    bool send(char* msg, struct sockaddr_in* dest) {

      if (OO >= 4) {
        std::cout << "fl_s sending buffer '" << msg << "'\n";
      }

      ssize_t bytes_sent = sendto(sock, msg, strlen(msg), 0,
        reinterpret_cast<sockaddr*>(dest), sizeof(*dest));

      if (bytes_sent < 0) {
        std::cout << "fl_s couldn't send, errno " << errno << ", " << strerror(errno) << std::endl;
        return false;
      }
      sent++;
      return true;
    }

    void bind_address(sockaddr_in* address) {
      if (bind(sock, reinterpret_cast<sockaddr*>(address), sizeof(*address)) < 0) {
        perror("couldn't bind socket of receiver\n");
        close(sock);
        exit(-1);
      }
    }

    void listen() {
      if (OO >= 1) std::cout << "listening on fairloss..." << std::endl;
      sockaddr_in from;
      socklen_t from_len = sizeof(from);
      ssize_t msg_len;
      bool succeeded;
      
      while (true) {
        char* buffer = static_cast<char*>(malloc(MAX_RECVD));
        if (OO >= 4) std::cout << "fl listening..." << std::endl;
        msg_len = recvfrom(sock, buffer, MAX_RECVD, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (msg_len < 0 || msg_len > MAX_RECVD) {
          perror("reading error...\n");
          if (OO >= 1) std::cerr << "weird msg_len: " << msg_len << " buffer " << buffer << std::endl;
          // close(sock);
          // exit(-1);
        } else {
          buffer[msg_len] = '\0';  // add null terminator
          if (OO >= 4) std::cout << "fl_r " << buffer << std::endl;
          recv++;
          succeeded = msg_queue.try_enqueue(std::pair(buffer, msg_len));
          if (succeeded)
            recv_in_q++;
        }
      }
    }

    void receiveWorker() {
      while (true) {
        std::pair<char*, ssize_t> pair;
        bool succeeded = msg_queue.try_dequeue(pair);
        if (!succeeded) {
          if (OO >= 3) std::cout << "sleep in receive" << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          continue;
        }
        app_receive(pair.first, pair.second);
      }
    }

  private:
    moodycamel::ReaderWriterQueue<std::pair<char*, ssize_t>> msg_queue;
    std::function<void(char*, ssize_t)> app_receive;
    int sock;

    unsigned int sent = 0;
    unsigned int recv = 0;
    unsigned int recv_in_q = 0;
    unsigned int last_sent = 0;
    unsigned int last_recv = 0;
    unsigned int last_recv_in_q = 0;
};