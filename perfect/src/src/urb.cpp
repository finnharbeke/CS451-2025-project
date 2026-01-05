#pragma once

#include <iostream>
#include <functional>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "beb.cpp"
#include "msg_codec.cpp"

class URB {
  public:
    URB(unsigned char id_, unsigned char n_,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<bool()> app_send,
      std::function<void(unsigned char, char*, char*)> app_receive
    ) :
    beb(
      BEB(id_, n_, addrs_,
        std::bind(&URB::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    ), id(id_), n(n_), app_send(app_send), app_receive(app_receive) {
        for (unsigned char p = 1; p <= n; p++) {
            pending.try_emplace(p); // constructs default IT inplace
            pending_mutxs.try_emplace(p);
            ack.try_emplace(p);
            delivered.try_emplace(p);
        }
    }

    void bind_address(sockaddr_in* address) {
        beb.bind_address(address);
    }

    void enqueueWorker() {
        bool done = app_send();
        while (!done) {
            beb.await_ready_for_more();
            done = app_send();
        }
        if (OO >= 1) std::cout << "done sending" << std::endl;
    }
    
    void sendWorker() {
        beb.sendWorker();
    }
    
    void ackWorker() {
        beb.ackWorker();
    }
    
    void receiveWorker() {
        beb.receiveWorker();
    }
    
    void heartbeats() {
        beb.heartbeats();
    }

    void stats() {
        std::cout << sent - last_sent << "," << recv - last_recv << ",";
        beb.stats();
        last_sent = sent;
        last_recv = recv;
    }

    void send(char* msg) {
        auto msg_id = msg_id_counter++;
        {
            std::lock_guard lock(pending_mutxs[id]);
            pending[id].emplace(msg_id);
        }
        codec::add_urb_msg_sender_n_id(msg, id, msg_id);
        beb.send(msg);
        sent++;
    }

    void listen() {
        beb.listen();
    }

    void receive(unsigned char beb_sender, char* msg, char* end) {
        // accessed by both send and receive thread
        char* ptr = msg;
        unsigned char sender = static_cast<unsigned char>(*ptr - '0');
        unsigned int msg_id = static_cast<unsigned int>(strtoul(ptr+1, &ptr, 16));
        ptr++;
        if (OO >= 2)
            std::cout << "urb_r " << static_cast<short>(sender) << "-" << msg_id << std::endl;

        if (ack[sender].find(msg_id) == ack[sender].end()) {
            ack[sender][msg_id] = 0;
        }
        ack[sender][msg_id] += 1;

        {
            std::lock_guard lock(pending_mutxs[sender]);
            if (pending[sender].find(msg_id) == pending[sender].end()) {
                pending[sender].emplace(msg_id);
                char* new_msg = codec::beb_from_other(msg);
                beb.send(new_msg);
            }
        }

        if (ack[sender][msg_id] > n/2 && delivered[sender].find(msg_id) == delivered[sender].end()) {
            delivered[sender].emplace(msg_id);
            app_receive(sender, ptr, end);
        }
        recv++;
    }

  private:
    BEB beb;
    unsigned char id;
    unsigned char n;
    std::function<bool()> app_send;
    std::function<void(unsigned char, char*, char*)> app_receive;

    std::unordered_map<unsigned char, std::unordered_set<unsigned int>> pending;
    std::unordered_map<unsigned char, std::mutex> pending_mutxs;
    
    std::unordered_map<unsigned char, std::unordered_map<unsigned int, unsigned char>> ack;
    
    std::unordered_map<unsigned char, std::unordered_set<unsigned int>> delivered;

    unsigned int msg_id_counter = 1;
    unsigned int sent = 0;
    unsigned int recv = 0;

    unsigned int last_sent = 0;
    unsigned int last_recv = 0;
};