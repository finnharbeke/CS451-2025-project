#pragma once

#include <iostream>
#include <functional>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "stubborn.cpp"
#include "log_queue.cpp"

class Perfect {
  public:
    Perfect(unsigned char id_, std::unordered_map<unsigned char, struct sockaddr_in>* addrs_) : st(Stubborn(id_, addrs_)) {}

    void bind_address(sockaddr_in* address) {
      st.bind_address(address);
    }

    void run(std::function<bool()> send_callback, std::function<void(unsigned char, char*)> receive_callback) {
      std::thread send(&Perfect::new_messages, this, send_callback);
      send.detach();
      std::thread work1([&]{ st.send_messages(); });
      work1.detach();
      std::thread work2([&]{ st.send_acks(); });
      work2.detach();
      std::thread listen(&Perfect::listen, this);
      listen.detach();
      std::thread receiving([this, receive_callback]{ // capture by value not reference, other
        auto fun = std::bind(&Perfect::receive, this, receive_callback,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        this->st.receive(fun);
      });
      receiving.detach();
    }

    void stats() {
      std::cout << sent - last_sent << "," << recv - last_recv << ",";
      st.stats();
      last_sent = sent;
      last_recv = recv;
    }

    void new_messages(std::function<bool()> callback) {
      bool done = callback();
      while (!done) {
        st.await_ready_for_more();
        done = callback();
      }
      if (OO >= 1) std::cout << "done sending" << std::endl;
    }

    void send(unsigned int msg_id, char* msg, struct sockaddr_in* dest) {
      sent++;
      st.send(msg_id, msg, dest);
    }

    void listen() {
      st.listen();
    }

    void receive(std::function<void(unsigned char, char*)> callback, unsigned char sender, char* msg, char* end) {
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
        callback(sender, sub_msg);

        if (end != sep)
          sep++;
      }
    }

  private:
    Stubborn st;
    unsigned int sent = 0;
    unsigned int recv = 0;

    unsigned int last_sent = 0;
    unsigned int last_recv = 0;
};