#pragma once

#include <iostream>
#include <functional>
#include <unordered_set>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>

#include <queue>
#include <mutex>
#include <condition_variable>

#include "global.h"
#include "fairloss.cpp"
#include "interval_tree.cpp"

struct StubbornMsg {
  unsigned int msg_id;
  char* msg;
  struct sockaddr_in* dest;
  std::chrono::steady_clock::time_point next_send;
  unsigned char back_off = INIT_BACKOFF;
  StubbornMsg(unsigned int msg_id, char* msg, sockaddr_in* dest) :
    msg_id(msg_id), msg(msg), dest(dest),
    next_send(std::chrono::steady_clock::now()) {}

  bool operator<(const StubbornMsg& other) const {
    return (next_send < other.next_send);
  }
};

class Stubborn {
  public:
    Stubborn(unsigned char id_, std::unordered_map<unsigned char, struct sockaddr_in>* addrs_)
    : id(id_), addrs(addrs_) {
      for (auto& [sender, _]: *addrs) {
        if (sender == id) continue;
        pending_acks[sender] = IntervalTree();
      }
    }

    void stats() {
      std::cout << sent - last_sent << "," << send_cycles - last_send_cycles << ","
        << s_ack - last_s_ack << "," << r_ack - last_r_ack << ","
        << ack_cycles - last_ack_cycles << "," << r_msg - last_r_msg << ","
        << recv - last_recv << "," << s_ackack - last_s_ackack << ","
        << r_ackack - last_r_ackack << ",";
      
      last_sent = sent;
      last_send_cycles = send_cycles;
      last_s_ack = s_ack;
      last_r_ack = r_ack;
      last_ack_cycles = ack_cycles;
      last_r_msg = r_msg;
      last_recv = recv;
      last_s_ackack = s_ackack;
      last_r_ackack = r_ackack;
      fl.stats();
    }

    void send(unsigned int msg_id, char* msg, struct sockaddr_in* dest) {
      std::lock_guard<std::mutex> lock(Q_mutx);
      Q.emplace(msg_id, msg, dest);
      cv_empty.notify_one();
    }

    void send_messages() {
      if (OO >= 1) std::cout << "running stubborn" << std::endl;
      while (true) {
        next();
        send_cycles++;
      }
    }

    void next() {
      {
        std::unique_lock<std::mutex> lock(Q_mutx);
        cv_empty.wait(lock, [&]{ return !Q.empty(); });
      }

      // sleep
      std::this_thread::sleep_until((*Q.begin()).next_send);
      
      StubbornMsg stbmsg = StubbornMsg(0, nullptr, nullptr);
      {
        std::unique_lock<std::mutex> lock(Q_mutx);
        auto top = Q.begin();
        stbmsg = *top;
        Q.erase(top);
        auto it = acked.find(stbmsg.msg_id);
        if (it != acked.end()) {
          // old message, throw out
          {
            std::unique_lock<std::mutex> lock(ackedset_mutx);
            acked.erase(it);
          }
          cv_ready.notify_one(); // Q size decreased
          return;
        }
        
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        stbmsg.next_send = now + std::chrono::milliseconds(1 << stbmsg.back_off);
        if (stbmsg.back_off < MAX_BACKOFF)
          stbmsg.back_off++;
  
        Q.insert(stbmsg);
      }
      if (OO >= 2) std::cout << "st_s " << stbmsg.msg_id << std::endl;
      fl.send(stbmsg.msg, stbmsg.dest);
      sent++;
      
    }

    void await_ready_for_more() {
      std::unique_lock<std::mutex> lock(Q_mutx);
      cv_ready.wait(lock, [&](){
        // std::cout << "checking " << Q.size() << std::endl;
        return Q.size() < REFILL;
      });
    }

    void bind_address(sockaddr_in* address) {
      fl.bind_address(address);
    }

    void receive(std::function<void(unsigned char, char*, char*)> callback, ssize_t msg_len, char* buffer) {
      if (OO >= 3) std::cout << "st got smth" << std::endl;
      recv++;
      char* end = buffer + msg_len;
      unsigned char sender = static_cast<unsigned char>(*buffer - '0');
      
      char second = *(buffer + 1);
      if (second == 6) {
        // ack
        char third = *(buffer + 2);
        if (third == 6) {
          // ackack
          receive_ackack(sender, buffer+3, end);
        } else {
          receive_ack(sender, buffer+2, end);
        }
      } else {
        receive_msg(callback, sender, buffer+1, end);
      }
    }

    static unsigned long hash_msg(unsigned char sender, unsigned int msg_id) {
      return (static_cast<unsigned long>(sender) << 32) | msg_id;
    }

    void receive_msg(std::function<void(unsigned char, char*, char*)> callback, unsigned char sender, char* buffer, char* end) {
      r_msg++;
      char* msg = nullptr;
      unsigned int msg_id = static_cast<unsigned int>(strtoul(buffer, &msg, 16));
      msg++;
      unsigned long timestamp_ul = strtoul(buffer, &msg, 16);
      auto timestamp_dur = std::chrono::steady_clock::duration(timestamp_ul);
      auto timestamp = std::chrono::steady_clock::time_point(timestamp_dur);
      msg++;
      if (OO >= 2)
        std::cout << "st_r " << static_cast<short>(sender)  << " " << msg_id << std::endl;
      if (timestamp < time_cutoff)
        return;
      auto hash = hash_msg(sender, msg_id);
      if (lookup.find(hash) == lookup.end()) {
        lookup.emplace(hash);
        add_to_ack(sender, msg_id);
        callback(sender, msg, end);
      }
    }

    void add_to_ack(unsigned char sender, unsigned int msg_id) {
      // std::lock_guard<std::mutex> lock(ack_mutx);
      if (!pending_acks[sender].insert(msg_id) && OO >= 1)
        std::cerr << "ack insertion failed: " << msg_id << " on sender " << static_cast<short>(sender) << std::endl;
    }

    void send_acks() {
      if (OO >= 1) std::cout << "running acks in stubborn" << std::endl;
      while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ACK_INTERVAL_MILLIS));

        for (auto& [sender, tree] : pending_acks) {
          sockaddr_in* dest = &(*addrs)[sender];
          auto it = tree.cbegin();
          const auto end = tree.cend();
          if (OO >= 1) std::cout << "acking to "<< static_cast<short>(sender) << ": {";
          for (; it != end; it++) {
            char* msg = compose_ack(sender, *it);
            if (OO >= 1) std::cout << "[" << it->left << "," << it->right << "], ";
            
            s_ack++;
            fl.send(msg, dest);
          };
          if (OO >= 1) std::cout << "}" << std::endl;
        }
      }
    }

    char* compose_ack(unsigned char sender, Interval inter) {
      if (OO >= 3)
        std::cout << "composing ack to " << static_cast<int>(sender)
        << ", [" << inter.left << "," << inter.right << "]" << std::endl;
      
      char* buffer = static_cast<char*>(malloc(ACK_LEN));
      char* ptr = buffer;
      // sender id
      *ptr = static_cast<char>(id + '0'); // nicer
      ++ptr;

      *ptr = 6; // ack
      ++ptr;

      snprintf(ptr, _CMPRSD_S, "%x", inter.left);
      while (*ptr != 0)
        ++ptr;
      *ptr = 31; // ascii unit separator
      ++ptr;

      snprintf(ptr, _CMPRSD_S, "%x", inter.right);
      
      return buffer;
    }

    void receive_ack(unsigned char sender, char* msg, char* end) {
      r_ack++;

      char* sep = msg;
      
      // COMPOSE ACKACK
      // char + char + char + ack_id + null
      // id | ack | ack | ack_id
      char* buffer = static_cast<char*>(malloc(ACKACK_LEN));
      char* ptr = buffer;
      
      *ptr = static_cast<char>(id + '0');
      ++ptr;
      
      *ptr = 6; // ack
      ++ptr;
      *ptr = 6; // ack
      ++ptr;
      
      // TODO could just copy msg to ptr...
      unsigned int left = static_cast<unsigned int>(strtoul(sep, &sep, 16));
      snprintf(ptr, _CMPRSD_S, "%x", left);
      while (*ptr != 0)
        ++ptr;
      *ptr = 31; // ascii unit separator
      ++ptr;

      sep++;
      unsigned int right = static_cast<unsigned int>(strtoul(sep, &sep, 16));
      snprintf(ptr, _CMPRSD_S, "%x", right);

      if (OO >= 2)
        std::cout << "st_r_a " << static_cast<short>(sender) << ": ["
          << left << ", " << right << "]" << std::endl;

      fl.send(buffer, &(*addrs)[sender]); // TODO check if multithreaded sending needs checks
      s_ackack++;

      // actually ACK

      std::lock_guard<std::mutex> lock(ackedset_mutx);
      for (auto msg_id = left; msg_id <= right; msg_id++) {
        if (OO >= 3)
          std::cout << "removing from sending: " << msg_id << std::endl;
        // remove msg_id from sending
        acked.emplace(msg_id);
      }
    }

    void receive_ackack(unsigned char sender, char* msg, char* end) {
      r_ackack++;
      char* head = msg;
      unsigned int left = static_cast<unsigned int>(strtoul(head, &head, 16));
      head++;
      unsigned int right = static_cast<unsigned int>(strtoul(head, nullptr, 16));
      if (OO >= 2)
        std::cout << "st_r_aa " << static_cast<short>(sender) << ": ["
          << left << "," << right << "]" << std::endl;

      time_cutoff = std::chrono::steady_clock::now();
      // remove from pending
      pending_acks[sender].erase(left, right);
      // remove from lookup
      for (auto msg_id = left; msg_id <= right; msg_id++) {
        lookup.erase(hash_msg(sender, msg_id));
      }
    }

    void listen(std::function<void(unsigned char, char*, char*)> callback) {
      auto fun = std::bind(&Stubborn::receive, this, callback,
        std::placeholders::_1, std::placeholders::_2);
      fl.listen(fun);
    }

  private:
    unsigned char id;
    FairLoss fl;
    std::chrono::steady_clock::time_point time_cutoff;
    std::unordered_set<unsigned long> lookup{};
    std::set<StubbornMsg> Q;
    std::unordered_set<unsigned int> acked; // could be interval tree but idk since single removal
    std::mutex ackedset_mutx;
    // std::queue<StubbornMsg*> Q;
    // std::unordered_map<unsigned int, StubbornMsg*> msg_index;
    std::mutex Q_mutx;
    // std::mutex ack_mutx;
    std::condition_variable cv_ready;
    std::condition_variable cv_empty;
    std::condition_variable cv_acks;
    std::unordered_map<unsigned char, struct sockaddr_in>* addrs;
    std::unordered_map<unsigned char, IntervalTree> pending_acks;

    unsigned int send_cycles = 0;
    unsigned int sent = 0;
    unsigned int ack_cycles = 0;
    unsigned int s_ack = 0;
    unsigned int s_ackack = 0;
    unsigned int recv = 0;
    unsigned int r_msg = 0;
    unsigned int r_ack = 0;
    unsigned int r_ackack = 0;
    unsigned int last_sent = 0;
    unsigned int last_send_cycles = 0;
    unsigned int last_s_ack = 0;
    unsigned int last_r_ack = 0;
    unsigned int last_ack_cycles = 0;
    unsigned int last_r_msg = 0;
    unsigned int last_recv = 0;
    unsigned int last_s_ackack = 0;
    unsigned int last_r_ackack = 0;
};