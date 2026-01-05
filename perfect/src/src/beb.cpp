#pragma once

#include <iostream>
#include <functional>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "perfect.cpp"
#include "msg_codec.cpp"

class BEB {
  public:
    BEB(unsigned char id_, unsigned char n_,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<void(unsigned char, char*, char*)> app_receive
    ) :
    pf(
      Perfect(id_, addrs_,
        std::bind(&BEB::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    ), id(id_), n(n_), app_receive(app_receive) {}

    void bind_address(sockaddr_in* address) {
      pf.bind_address(address);
    }

    void await_ready_for_more() {
      pf.await_ready_for_more();
    }
    
    void sendWorker() {
      pf.sendWorker();
    }
    
    void ackWorker() {
      pf.ackWorker();
    }
    
    void receiveWorker() {
      pf.receiveWorker();
    }
    
    void heartbeats() {
      pf.heartbeats();
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << ",";
      pf.stats();
      last_sent = sent;
      last_recv = recv;
    }

    void send(char* msg) {
        auto msg_id = msg_id_counter++;
        codec::add_beb_msg_sender_n_id(msg, id, msg_id);
        sent++;
        for (unsigned char p = 1; p <= n; p++) {
            // if (p == id) {
            //     receive()
            // } else {
            char* cpy = static_cast<char*>(malloc(PACKET_LEN));
            strcpy(cpy, msg);
            pf.send(msg_id, cpy, p); // might be better just to have no interleaving
        }
        free(msg);
    }

    void listen() {
      pf.listen();
    }

    void receive(unsigned char sender, char* msg, char* end) {
        // accessed by both send and receive thread
        app_receive(sender, msg, end);
        recv++;
    }

  private:
    Perfect pf;
    unsigned char id;
    unsigned char n;
    std::function<void(unsigned char, char*, char*)> app_receive;
    
    unsigned long msg_id_counter = 1;
    unsigned long sent = 0;
    unsigned long recv = 0;

    unsigned long last_sent = 0;
    unsigned long last_recv = 0;
};