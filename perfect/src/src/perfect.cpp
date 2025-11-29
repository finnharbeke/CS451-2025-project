#pragma once

#include <iostream>
#include <functional>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "stubborn.cpp"

class Perfect {
  public:
    Perfect(unsigned char id_,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<bool()> app_send,
      std::function<void(unsigned char, char*)> app_receive
    ) :
    st(
      Stubborn(id_, addrs_,
        std::bind(&Perfect::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    ), app_send(app_send), app_receive(app_receive) {}

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void enqueueWorker() {
      bool done = app_send();
      while (!done) {
        st.await_ready_for_more();
        done = app_send();
      }
      if (OO >= 1) std::cout << "done sending" << std::endl;
    }
    
    void sendWorker() {
      st.sendWorker();
    }
    
    void ackWorker() {
      st.ackWorker();
    }
    
    void receiveWorker() {
      st.receiveWorker();
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << ",";
      st.stats();
      last_sent = sent;
      last_recv = recv;
    }

    void send(unsigned int msg_id, char* msg, struct sockaddr_in* dest) {
      sent++;
      st.send(msg_id, msg, dest);
    }

    void listen() {
      st.listen();
    }

    void receive(unsigned char sender, char* msg, char* end) {
      // receives message formatted as
      // seq_nr | seq_nr | seq_nr ...
      recv++;
      char* sep = msg;
      if (OO >= 4)
        std::cout << "buffer (size " << (end-msg) << ") " << msg << std::endl;
      while (sep != end) {
        char* sub_msg = sep;
        if (OO >= 4)
          std::cout << "rest buffer " << sub_msg << std::endl;
        sep = std::find(sep, end, static_cast<char>(31));
        // end sub_msg (instead of unit separator 31)
        if (end != sep)
          *sep = '\0';

        // receive
        if (OO >= 3)
          std::cout << "pf_r " << static_cast<int>(sender)
            << ": " << sub_msg << std::endl;

        // receive callback
        app_receive(sender, sub_msg);

        if (end != sep)
          sep++;
      }
    }

  private:
    Stubborn st;
    std::function<bool()> app_send;
    std::function<void(unsigned char, char*)> app_receive;
    unsigned int sent = 0;
    unsigned int recv = 0;

    unsigned int last_sent = 0;
    unsigned int last_recv = 0;
};