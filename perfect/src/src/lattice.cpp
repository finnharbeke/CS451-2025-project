#pragma once

#include <iostream>
#include <unordered_map>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "beb.cpp"
#include "msg_codec.cpp"
#include "config.cpp"

struct SingleShot {
    unsigned char round = 0;
    unsigned char tally = 0;
    unsigned char response_count = 0;
    std::set<unsigned int>* accepted;
    std::set<unsigned int>* Vr = new std::set<unsigned int>();
    std::set<unsigned int>* learned;
    bool decided = false;
};

std::ostream& operator<<(std::ostream& os, std::set<unsigned int>* s);
std::ostream& operator<<(std::ostream& os, std::set<unsigned int>* s) {
    for (auto x : *s)
        os << x << " ";
    return os;
}

class LatticeAgreement {
  public:
    LatticeAgreement(unsigned char id_, unsigned char n_,
      LatticeConfig* conf,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<void(unsigned int, std::set<unsigned int>*)> dec_
    ) :
    beb(
      BEB(id_, n_, addrs_,
        std::bind(&LatticeAgreement::receive, this, std::placeholders::_1, std::placeholders::_2))
    ), id(id_), n(n_), config(conf), decide(dec_) {

        if (n % 2 == 0)
            f = (n >> 1) - 1;
        else
            f = n >> 1;

        for (unsigned char i = 1; i <= n; i++) {

        }
    }

    // shared resources
    // shots: start_batch (enqueueWorker), new_round (start_batch & receive), receive
    // config: only start_batch (enqueueWorker)

    void bind_address(sockaddr_in* address) {
        beb.bind_address(address);
    }

    void enqueueWorker() {
        bool done = start_batch();
        while (!done) {
            beb.await_ready_for_more();
            done = start_batch();
        }
        if (OO >= 1) std::cout << "done sending" << std::endl;
    }

    void new_round(unsigned int p_id) {

        auto shot = shots[p_id];
        shot->round++;
        
        // join Vr into accepted (before first Vr will be empty)
        shot->accepted->insert(
            shot->Vr->begin(), shot->Vr->end()
        );
        shot->Vr = new std::set<unsigned int>();

        shot->learned = new std::set<unsigned int>();
        shot->learned->insert(
            shot->accepted->begin(), shot->accepted->end()
        );

        if (OO >= 2) {
            std::cout << "new round " << static_cast<unsigned short>(shot->round) << " for " << p_id
            << "; learned = " << shot->learned << std::endl;
        }

        char* msg = codec::compose_proposal(p_id, shot->round, shot->accepted);
        
        shot->tally = 1; // acking myself
        shot->response_count = 1;


        beb.broadcast_others(msg);
    }

    bool start_batch() {
        if (next_slot > config->p)
            return true;

        while (next_to_prepare - next_slot <= SLOTS_AHEAD) {
            if (next_to_prepare > config->p)
                break;
            auto proposal = config->next_proposal();
    
            SingleShot* shot = new SingleShot;
            shot->accepted = proposal;
            
            shots[next_to_prepare] = shot;
            next_to_prepare++;
        }
        
        new_round(next_slot);

        next_slot++;
        return false;
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
        // std::cout << sent - last_sent << "," << recv - last_recv << ",";
        beb.stats();
        // last_sent = sent;
        // last_recv = recv;
    }

    void await_ready_for_more() {
        beb.await_ready_for_more();
    }

    // void send(char* msg) {
    //     beb.sendto(msg);
    // }

    void listen() {
        beb.listen();
    }

    void receive(unsigned char sender, char* msg) {
        std::cout << msg << std::endl;
        auto msg_type = *msg++;
        unsigned char round = static_cast<unsigned char>(*(msg++) - '0');
        unsigned int p_id = static_cast<unsigned int>(strtoul(msg, &msg, 16));

        auto it = shots.find(p_id);
        if (it == shots.end()) {
            // WHAT IF I HAVEN'T ARRIVED IN THIS P_ID YET!
            if (p_id >= next_to_prepare) {
                // sorry not ready for you
                char* msg = codec::compose_nak(p_id, round, new std::set<unsigned int>());
                beb.sendto(msg, sender);
                return;
            } else {
                if (OO >= 1) {
                    std::cout << "strange p_id or forgotten about: " << p_id << std::endl;
                }
                return;
            }

        }
        auto shot = it->second;

        std::set<unsigned int>* value = new std::set<unsigned int>;
        if (msg_type == 5 || msg_type == 21) {
            while (*msg == 31) {
                msg++;
                unsigned int x = static_cast<unsigned int>(strtoul(msg, &msg, 16));
                value->emplace(x);
            }
        }

        // NOW ACT UPON things

        if (msg_type == 6 || msg_type == 21) { // feedback
            if (shot->decided || round != shot->round)
                return;
            shot->response_count++;
            if (msg_type == 6) {// ack
                shot->tally++;
                if (OO >= 2)
                    std::cout << p_id << "-" << static_cast<unsigned short>(round)
                    << "<" << static_cast<unsigned short>(sender) << " ack, tally = " <<
                    shot->tally << "/" << shot->response_count << std::endl;
            } else {
                shot->Vr->insert(value->cbegin(), value->cend());
                if (OO >= 2)
                    std::cout << p_id << "-" << static_cast<unsigned short>(round)
                    << "<" << static_cast<unsigned short>(sender) << " nak, tally = "
                    << static_cast<unsigned short>(shot->tally) << "/" << static_cast<unsigned short>(shot->response_count)
                    << ", Vr = " << shot->Vr << std::endl;
            }

            if (shot->response_count >= n - f) { // end round
                if (shot->tally > (n >> 1)) {
                    // decide in order
                    shot->decided = true;
                    if (p_id == next_to_log) {
                        while (next_to_log < next_to_prepare &&
                            shots[next_to_log]->decided) {

                            decide(next_to_log, shots[next_to_log]->learned);
                            next_to_log++;
                        }
                    }
                } else
                    new_round(p_id);
            }
        } else { // proposal from sender
            if (OO >= 2)
                std::cout << p_id << "-" << static_cast<unsigned short>(round)
                    << ">" << static_cast<unsigned short>(sender) << " , "
                << value;
            bool is_superset = true;
            for (auto x : *(shot->accepted))
                if (value->find(x) == value->end()) {
                    is_superset = false;
                    break;
                }
            if (is_superset) {
                // send ack
                if (OO >= 2)
                    std::cout << "-> ACK" << std::endl;
                    
                char* msg = codec::compose_ack(p_id, round);
                beb.sendto(msg, sender);
                
                shot->accepted = value;
                    
            } else {
                // send nak
                if (OO >= 2)
                    std::cout << "-> NAK" << std::endl;
                char* msg = codec::compose_nak(p_id, round, shot->accepted);
                beb.sendto(msg, sender);
            }
        }
    }

  private:
    BEB beb;
    unsigned int next_slot = 1;
    unsigned int next_to_prepare = 1;
    unsigned int next_to_log = 1;
    unsigned char id, n, f;
    LatticeConfig* config;
    std::function<void(unsigned int, std::set<unsigned int>*)> decide;
    std::unordered_map<unsigned int, SingleShot*> shots;
};