#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>

#include <condition_variable>
#include <mutex>
#include <atomic>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "beb.cpp"
#include "msg_codec.cpp"
#include "config.cpp"

struct Window {

    std::atomic_uint loaded_front = 1; // exclusive
    std::atomic_uint active_front = 1; // exclusive
    std::atomic_uint active_tail = 0; // inclusive
    std::atomic_uint loaded_tail = 0; // inclusive

};

struct SingleShot {
    unsigned char round = 0;
    unsigned char tally = 0;
    unsigned char response_count = 0;
    std::vector<unsigned int> accepted;
    std::vector<unsigned int> Vr;
    std::vector<unsigned int> learned;
    bool decided = false;
    std::chrono::steady_clock::time_point last_answer = 
        std::chrono::steady_clock::now();
};

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned int>& s);
std::ostream& operator<<(std::ostream& os, const std::vector<unsigned int>& s) {
    auto it = s.begin();
    if (s.size() > 0) {
        os << *it;
        it++;
    }
    else
        return os;
    for (; it != s.end(); it++)
        os << " " << *it;
    return os;
}

void unionize(std::vector<unsigned int>& into, const std::vector<unsigned int>& from);
void unionize(std::vector<unsigned int>& into, const std::vector<unsigned int>& from) {
    std::vector<unsigned int> result;
    result.reserve(into.size() + from.size());

    std::set_union(into.begin(), into.end(),
                    from.begin(), from.end(),
                    std::back_inserter(result));

    into = std::move(result);
}

class LatticeAgreement {
  public:
    LatticeAgreement(unsigned char id_, unsigned char n_,
      LatticeConfig* conf,
      std::unordered_map<unsigned char, struct sockaddr_in>* addrs_,
      std::function<void(unsigned int, std::vector<unsigned int>&)> dec_
    ) :
    beb(
      BEB(id_, n_, addrs_,
        std::bind(&LatticeAgreement::receive, this, std::placeholders::_1, std::placeholders::_2))
    ), id(id_), p(n_), config(conf), decide(dec_) {

        if (OO >= 1)
            std::cout << "p = " << static_cast<short>(p) << std::endl;

        if (p % 2 == 0)
            f = (p >> 1) - 1;
        else
            f = p >> 1;

        if (OO >= 1)
            std::cout << "p - f = " << static_cast<short>(p-f) << std::endl;

        for (unsigned char i = 1; i <= p; i++) {

        }

        last_check_on_unanswered = std::chrono::steady_clock::now();
    }

    // shared resources
    // shots: start_batch (enqueueWorker), new_round (start_batch & receive), receive
    //    -> necessarily timewise apart though, can't receive smth about it before sending, which is right at the end of start_batch
    // config: only start_batch (enqueueWorker)

    void bind_address(sockaddr_in* address) {
        beb.bind_address(address);
    }

    void enqueueWorker() {
        bool tmp_done = start_batch();
        bool done = tmp_done;
        while (wd.active_tail <= config->n) {
            await_for_more_and_check_unanswered();
            
            tmp_done = start_batch();
            if (OO >= 1 && !done && tmp_done) std::cout << "done sending" << std::endl;
            done = tmp_done;
        }
        if (OO >= 1) std::cout << "done checking unanswereds" << std::endl;
    }

    void new_round(unsigned int p_id) {
        auto shot = shots[p_id];
        shot->round++;
        
        // join Vr into accepted (before first Vr will be empty)
        unionize(shot->accepted, shot->Vr);
        shot->Vr.clear();

        // copy
        shot->learned = shot->accepted;

        if (OO >= 2) {
            std::cout << static_cast<unsigned short>(id) << " -->> : " 
                << p_id << "-" << static_cast<unsigned short>(shot->round)
            << " prop (" << shot->learned << ")" << std::endl;
        }

        char* msg = codec::compose_proposal(p_id, shot->round, shot->learned);
        
        shot->tally = 1; // acking myself
        shot->response_count = 1;
        shot->last_answer = std::chrono::steady_clock::now();
        
        beb.broadcast_others(msg);

        rounds++;
    }

    void restart_round(unsigned int p_id) {
        restarts++;
        auto shot = shots[p_id];

        if (OO >= 2) {
            std::cout << static_cast<unsigned short>(id) << " -->> : " 
                << p_id << "-" << static_cast<unsigned short>(shot->round)
            << " re-prop (" << shot->learned << ")" << std::endl;
        }

        char* msg = codec::compose_proposal(p_id, shot->round, shot->learned);
        
        shot->tally = 1; // acking myself
        shot->response_count = 1;
        shot->last_answer = std::chrono::steady_clock::now();

        beb.broadcast_others(msg);

        rounds++;
    }

    bool start_batch() {
        {
            std::lock_guard<std::mutex> lock(window_shots_mutx);
            if (wd.active_front > config->n)
                return true;
    
            while (wd.loaded_front - wd.active_front < SLOTS_AHEAD + 1
                && wd.loaded_front <= config->n) {
                    
                SingleShot* shot = new SingleShot;
                shot->accepted.reserve(config->vs);
                config->next_proposal(shot->accepted);
                
                shots[wd.loaded_front] = shot;
                wd.loaded_front++;
                if (wd.loaded_tail == 0) // first time
                    wd.loaded_tail++;
            }
        }
            
        auto to_start = wd.active_front++;
        if (wd.active_tail == 0)
            wd.active_tail++;
        
        new_round(to_start);

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
        std::cout << decided - last_decided << "," << rounds - last_rounds << "," << restarts - last_restarts << ","
           << wd.loaded_tail << "," << wd.active_tail << "," << wd.active_front << "," << wd.loaded_front << ",";
        // std::cout << sent - last_sent << "," << recv - last_recv << ",";
        beb.stats();
        last_decided = decided;
        last_rounds = rounds;
        last_restarts = restarts;
    }

    void await_for_more_and_check_unanswered() {
        std::unique_lock<std::mutex> lock(window_shots_mutx);
        cv_window.wait_for(lock,
            std::chrono::milliseconds(CHECK_ON_UNANSWERED_MILLIS), [&]{

            check_on_unanswered();
            return wd.active_front - wd.active_tail < MAX_ACTIVE_WINDOW;
        });
    }

    // void send(char* msg) {
    //     beb.sendto(msg);
    // }

    void listen() {
        beb.listen();
    }

    void check_on_unanswered() {
        // mutx is owned
        auto now = std::chrono::steady_clock::now();
        auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_check_on_unanswered);
        if (passed.count() <= CHECK_ON_UNANSWERED_MILLIS)
            return;
        

        for (auto& [p_id, shot] : shots) {
            if (shot->decided || p_id >= wd.active_front)
                continue;
            passed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - shot->last_answer);

            if (passed.count() > RETRY_UNANSWERED_MILLIS) {
                restart_round(p_id);
            }
        }
    }

    void receive(unsigned char sender, char* msg) {
        auto msg_start = msg;
        auto msg_type = *msg++;
        unsigned char round = static_cast<unsigned char>(*(msg++) - '0');
        unsigned int p_id = static_cast<unsigned int>(strtoul(msg, &msg, 16));

        std::vector<unsigned int> value;
        if (msg_type == 5 || msg_type == 21) {
            while (*msg == 31) {
                msg++;
                unsigned int x = static_cast<unsigned int>(strtoul(msg, &msg, 16));
                value.push_back(x);
            }

            if (value.size() == 0) {
                std::cerr << "empty value, msg: " << msg_start << std::endl << std::endl;
            }
        }

        std::unordered_map<unsigned int, SingleShot *>::iterator it;
        {
            std::lock_guard<std::mutex> lock(window_shots_mutx);
            it = shots.find(p_id);
        }
        if (it == shots.end()) {
            // WHAT IF I HAVEN'T ARRIVED IN THIS P_ID YET!
            bool too_far;
            {
                std::lock_guard<std::mutex> lock(window_shots_mutx);
                too_far = p_id >= wd.loaded_front;
            }
            if (too_far) {
                // sorry not ready for you
                if (msg_type == 5) {
                    if (OO >= 2)
                        std::cout << static_cast<unsigned short>(id) << " <-- " << static_cast<unsigned short>(sender)
                        << ": " << p_id << "-" << static_cast<unsigned short>(round)
                        << " prop (" << value << ") -> ignore, not ready" << std::endl;
                } else {
                    // what impossible??
                    std::cerr << static_cast<unsigned short>(id) << " <-> " << static_cast<unsigned short>(sender)
                    << ": " << p_id << "-" << static_cast<unsigned short>(round);
                    if (msg_type == 6)
                        std::cerr << " ack";
                    else
                        std::cerr << " nak (" << value << ")";
                    std::cerr << " -> what the helly" << std::endl;
                }
                
            } else {
                std::cerr << static_cast<unsigned short>(id) << " <-- " << static_cast<unsigned short>(sender)
                << ": " << p_id << "-" << static_cast<unsigned short>(round)
                << " prop (" << value << ")\nstrange p_id or forgotten about: " << p_id << ", my loaded tail = " << wd.loaded_tail 
                << ", my loaded front = " << wd.loaded_front << std::endl;
                std::cerr << "msg: " << msg_start << std::endl;
                // for (auto it2 = shots.begin(); it2 != shots.end(); it2++)
                //     std::cerr << it2->first << "(" << static_cast<unsigned short>(it2->second->round) << ";" << it2->second->decided << ") ";
                std::cerr << std::endl;
                std::cerr << std::endl;
            }
            return;
        }
        auto shot = it->second;

        // NOW ACT UPON things

        if (msg_type == 6 || msg_type == 21) { // feedback
            if (shot->decided || round != shot->round) {

                if (OO >= 2) {
                    std::cout << static_cast<unsigned short>(id) << " <-> " << static_cast<unsigned short>(sender) << ": " << p_id << "-"
                    << static_cast<unsigned short>(round) << (msg_type == 6 ? " ack" : " nak")
                    << " ignored bc " << (shot->decided ? "decided" : "my round = ");
                    if (!shot->decided)
                        std::cout << static_cast<unsigned short>(shot->round);
                    std::cout << std::endl;
                }
                return;
            }
            shot->response_count++;
            auto now = std::chrono::steady_clock::now();
            shot->last_answer = now;
            if (msg_type == 6) {// ack
                shot->tally++;
                if (OO >= 2)
                    std::cout << static_cast<unsigned short>(id) << " <-> " << static_cast<unsigned short>(sender) << ": " << p_id << "-"
                    << static_cast<unsigned short>(round) << " ack, tally = " <<
                    static_cast<unsigned short>(shot->tally) << "/" << static_cast<unsigned short>(shot->response_count) << std::endl;
            } else {
                unionize(shot->Vr, value);
                if (OO >= 2)
                    std::cout << static_cast<unsigned short>(id) << " <-> " << static_cast<unsigned short>(sender) << ": " << p_id << "-"
                    << static_cast<unsigned short>(round) << " nak, tally = "
                    << static_cast<unsigned short>(shot->tally) << "/" << static_cast<unsigned short>(shot->response_count)
                    << ", value (" << value << ")" << " -> Vr (" << shot->Vr << ")" << std::endl;
            }

            if (shot->response_count >= p - f) { // end round
                if (shot->tally > (p >> 1)) {
                    // decide in order
                    shot->decided = true;
                    decided++;
                    if (OO >= 2)
                        std::cout << "deciding " << p_id << "!" << std::endl;
                    {
                        std::lock_guard<std::mutex> lock(window_shots_mutx);
                        if (p_id == wd.active_tail) {
                            while (wd.active_tail < wd.loaded_front &&
                                shots[wd.active_tail]->decided) {
    
                                decide(wd.active_tail, shots[wd.active_tail]->learned);
                                wd.active_tail++;
                                // such that there are exactly sweep_after many in tail
                                if (wd.active_tail - wd.loaded_tail > SWEEP_AFTER) {
                                    auto old = shots.find(wd.loaded_tail);
                                    // delete old->second->accepted;
                                    // delete old->second->learned;
                                    // delete old->second->Vr;
                                    delete old->second;
                                    shots.erase(old);
                                    wd.loaded_tail++;
                                }
                            }
                        }
                    }
                    cv_window.notify_one();
                } else
                    new_round(p_id);
            }
        } else { // proposal from sender
            if (OO >= 2)
                std::cout << static_cast<unsigned short>(id) << " <-- " << static_cast<unsigned short>(sender)
                    << ": " << p_id << "-" << static_cast<unsigned short>(round)
                    << " prop (" << value << ") ";
            bool is_superset = std::includes(value.begin(), value.end(),
                  shot->accepted.begin(), shot->accepted.end());
            if (is_superset) {
                // send ack
                if (OO >= 2)
                    std::cout << "-> ACK" << std::endl;
                    
                char* msg = codec::compose_ack(p_id, round);
                beb.sendto(msg, sender);
                
                // accepted = value
                shot->accepted = value;
                    
            } else {
                // send nak
                if (OO >= 2)
                    std::cout << "-> NAK (" << shot->accepted << ")" << std::endl;
                char* msg = codec::compose_nak(p_id, round, shot->accepted);
                beb.sendto(msg, sender);
            }
        }
    }

  private:
    BEB beb;
    // std::mutex shots_mutx;
    std::mutex window_shots_mutx;
    std::condition_variable cv_window;
    Window wd;
    unsigned char id, p, f;
    LatticeConfig* config;
    std::function<void(unsigned int, std::vector<unsigned int>&)> decide;
    std::unordered_map<unsigned int, SingleShot*> shots;
    std::chrono::steady_clock::time_point last_check_on_unanswered;

    unsigned long decided, last_decided;
    unsigned long rounds, last_rounds;
    unsigned long restarts, last_restarts;
};