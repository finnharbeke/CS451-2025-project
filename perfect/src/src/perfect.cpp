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
      std::function<void(unsigned char, char*, char*)> app_receive
    ) :
    st(
      Stubborn(id_, addrs_,
        std::bind(&Perfect::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    ), app_receive(app_receive) {}

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void await_ready_for_more() {
      st.await_ready_for_more();
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
    
    void heartbeats() {
      st.heartbeats();
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << ",";
      st.stats();
      last_sent = sent;
      last_recv = recv;
    }

    void send(unsigned long msg_id, char* msg, unsigned char dest) {
      sent++;
      st.send(msg_id, msg, dest);
    }

    void listen() {
      st.listen();
    }

    void receive(unsigned char sender, char* msg, char* end) {
      recv++;
      app_receive(sender, msg, end);
    }

    void contin() {
      st.contin();
    }

  private:
    Stubborn st;
    std::function<void(unsigned char, char*, char*)> app_receive;
    unsigned long sent = 0;
    unsigned long recv = 0;

    unsigned long last_sent = 0;
    unsigned long last_recv = 0;
};