#pragma once

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "readerwriterqueue.h"

class FairLoss {
  public:
    FairLoss(std::function<void(char*)> app_receive) :
      msg_queue(moodycamel::ReaderWriterQueue<char*>(MSGQSIZE)),
      app_receive(app_receive) {
      if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("couldn't create socket");
        exit(-1);
      }
      // setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &RCVBUFSIZE, sizeof(RCVBUFSIZE));
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << "," << recv_in_q - last_recv_in_q << "," << msg_queue.size_approx() << std::endl;
      last_sent = sent;
      last_recv = recv;
      last_recv_in_q = recv_in_q;
    }

    bool send(char* msg, struct sockaddr_in* dest) {

      if (OO >= 6) {
        std::cout << "fl_s sending buffer '" << msg << "'\n";
      }

      ssize_t bytes_sent = sendto(sock, msg, std::strlen(msg), 0,
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

      char* buffer = static_cast<char*>(malloc(MAX_RECVD));
      
      while (true) {
        if (OO >= 6) std::cout << "fl listening..." << std::endl;
        msg_len = recvfrom(sock, buffer, MAX_RECVD, 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (msg_len < 0 || msg_len > MAX_RECVD) {
          perror("reading error...\n");
          std::cerr << "weird msg_len: " << msg_len << " buffer " << buffer << std::endl;
          // close(sock);
          // exit(-1);
        } else {
          buffer[msg_len] = '\0';  // add null terminator
          if (OO >= 6) std::cout << "fl_r " << buffer << std::endl;
          recv++;
          char* msg = static_cast<char*>(malloc(std::strlen(buffer) + 1));
          std::strcpy(msg, buffer);
          succeeded = msg_queue.try_enqueue(msg);
          if (succeeded)
            recv_in_q++;
          else
            free(msg);
        }
      }
    }

    void receiveWorker() {
      while (true) {
        char* msg;
        bool succeeded = msg_queue.try_dequeue(msg);
        if (!succeeded) {
          if (OO >= 5) std::cout << "sleep in receive" << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          continue;
        }
        app_receive(msg);
        free(msg);
      }
    }

  private:
    moodycamel::ReaderWriterQueue<char*> msg_queue;
    std::function<void(char*)> app_receive;
    int sock;

    unsigned long sent = 0;
    unsigned long recv = 0;
    unsigned long recv_in_q = 0;
    unsigned long last_sent = 0;
    unsigned long last_recv = 0;
    unsigned long last_recv_in_q = 0;
};