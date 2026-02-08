#pragma once

#include <iostream>
#include <set>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "urb.cpp"
#include "msg_codec.cpp"

struct PendingMsg {
    unsigned int msg_id;
    unsigned char sender;
    char* msg;
    PendingMsg(unsigned int id_, unsigned char s_, char* m_) : msg_id(id_), sender(s_), msg(m_) {}

    bool operator<(const PendingMsg& other) const noexcept {
        return msg_id < other.msg_id;
    }
    bool operator<(const unsigned int& a) const noexcept {
        return msg_id < a;
    }
};

bool operator<(const unsigned int& a, const PendingMsg& it) noexcept;
bool operator<(const unsigned int& a, const PendingMsg& it) noexcept {
    return a < it.msg_id;
}

std::ostream& operator<<(std::ostream& out, const PendingMsg& it) noexcept;
std::ostream& operator<<(std::ostream& out, const PendingMsg& it) noexcept {
    return out << it.msg_id;
}

class FIFO {
  public:
    FIFO(unsigned char id_, unsigned char n_,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<void(unsigned char, char*)> app_receive
    ) :
    urb(
      URB(id_, n_, addrs_,
        std::bind(&FIFO::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    ), id(id_), n(n_), app_receive(app_receive) {
        
        for (unsigned char p = 1; p <= n; p++) {
            next.emplace(p, 1);
            pending.try_emplace(p);
        }
    }

    void bind_address(sockaddr_in* address) {
        urb.bind_address(address);
    }
    
    void sendWorker() {
        urb.sendWorker();
    }
    
    void ackWorker() {
        urb.ackWorker();
    }
    
    void receiveWorker() {
        urb.receiveWorker();
    }
    
    void heartbeats() {
        urb.heartbeats();
    }

    void stats() {
        // std::cout << sent - last_sent << "," << recv - last_recv << ",";
        urb.stats();
        // last_sent = sent;
        // last_recv = recv;
    }

    void await_ready_for_more() {
        urb.await_ready_for_more();
    }

    void send(char* msg) {
        urb.send(msg);
    }

    void listen() {
        urb.listen();
    }

    void receive(unsigned char urb_sender, unsigned int msg_id, char* msg) {

        char* payload = static_cast<char*>(malloc(PAYLOAD_LEN));
        strcpy(payload, msg);

        pending[urb_sender].emplace(msg_id, urb_sender, payload);

        auto top = pending[urb_sender].begin();
        auto print = [&]
        {
            std::cout << "pending-" << urb_sender + '0' << " next = " << next[urb_sender] << "; top " << *top << ", { ";
            for (auto m : pending[urb_sender])
                std::cout << m << ' ';
            std::cout << "}\n";
        };
        if (OO >= 5)
            print();
        
        while (next[urb_sender] == top->msg_id) {
            app_receive(top->sender, top->msg);
            free(top->msg);
            next[urb_sender]++;
            top = pending[urb_sender].erase(top);

            if (OO >= 5)
                print();
        }
    }

  private:
    URB urb;
    unsigned char id, n;
    std::function<void(unsigned char, char*)> app_receive;

    std::unordered_map<unsigned char, unsigned int> next;
    std::unordered_map<unsigned char, std::set<PendingMsg, std::less<>>> pending;
};