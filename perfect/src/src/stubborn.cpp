#pragma once

#include <iostream>
#include <functional>
#include <unordered_set>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include <queue>
#include <mutex>
#include <condition_variable>

#include "global.h"
#include "fairloss.cpp"

struct StubbornMsg {
  bool acked = false;
  char* msg;
  struct sockaddr_in* dest;
  std::chrono::steady_clock::time_point next_send;
  unsigned char back_off = 0;
  StubbornMsg(char* msg, sockaddr_in* dest) : msg(msg), dest(dest),
    next_send(std::chrono::steady_clock::now()) {}
};

class Stubborn {
  public:
    Stubborn(unsigned char id_, std::unordered_map<unsigned char, struct sockaddr_in>* addrs_)
    : id(id_), addrs(addrs_) {
      for (auto& [sender, _]: *addrs) {
        tobeacked[sender] = std::vector<unsigned int>();
        pending_acks[sender] = std::unordered_map<unsigned int, char*>();
        last_add_to_pending[sender] = std::chrono::steady_clock::now();
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
      StubbornMsg* stbmsg = new StubbornMsg(msg, dest);
      std::lock_guard<std::mutex> lock(Q_mutx);
      Q.push(stbmsg);
      msg_index[msg_id] = stbmsg;
      cv_empty.notify_one();
    }

    void send_messages() {
      if (OO >= 3) std::cout << "running stubborn" << std::endl;
      while (true) {
        if (OO >= 3) std::cout << "run loop" << std::endl;
        // run_acks();
        next();
        send_cycles++;
      }
    }

    void next() {
      {
        std::unique_lock<std::mutex> lock(Q_mutx);
        cv_empty.wait(lock, [&]{ return !Q.empty(); });
      }
      
      std::unique_lock<std::mutex> lock(Q_mutx);
      std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
      StubbornMsg* stbmsg = Q.front();
      Q.pop();
      if (stbmsg->acked) {
        cv_ready.notify_one();
        return;
      }
      if (stbmsg->next_send < now) {
        sent++;
        fl.send(stbmsg->msg, stbmsg->dest);
        stbmsg->next_send = now + std::chrono::milliseconds(1 << (3 + stbmsg->back_off));
        stbmsg->back_off++;
      }
      Q.push(stbmsg);
    }

    void await_ready_for_more() {
      std::unique_lock<std::mutex> lock(Q_mutx);
      cv_ready.wait(lock, [&](){
        // std::cout << "checking " << Q.size() << std::endl;
        return Q.size() < (MAX_PENDING >> 1);
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

    void receive_msg(std::function<void(unsigned char, char*, char*)> callback, unsigned char sender, char* buffer, char* end) {
      r_msg++;
      char* msg = nullptr;
      unsigned int msg_id = static_cast<unsigned int>(strtoul(buffer, &msg, 16));
      msg++;
      if (OO >= 1)
        std::cout << "st_r " << static_cast<short>(sender)  << " " << msg_id << std::endl;
      auto hash = (static_cast<unsigned long>(sender) << 32) | msg_id;
      if (lookup.find(hash) == lookup.end()) {
        lookup.emplace(hash);
        add_to_ack(sender, msg_id);
        callback(sender, msg, end);
      }
    }

    void add_to_ack(unsigned char sender, unsigned int msg_id) {
      std::lock_guard<std::mutex> lock(ack_mutx);
      tobeacked[sender].push_back(msg_id);
      if (tobeacked[sender].size() >= 8) {
        compose_ack(sender);
      }
    }

    void compose_ack(unsigned char sender) {
      unsigned int ack_id = ++ack_count;
      if (OO >= 2) std::cout << "composing ack to " << static_cast<int>(sender) << ", ack_id: " << ack_id << std::endl;
      last_add_to_pending[sender] = std::chrono::steady_clock::now();
      // char + char + ack_id + (at most) 8 * (hex_int) + null
      // id | ack | int | ...
      const unsigned char n = sizeof(unsigned int) + 1;
      char* buffer = static_cast<char*>(malloc(1 + 1 + 3*n + 8 * 3*n + 1));
      char* ptr = buffer;
      // sender id
      *ptr = static_cast<char>(id + '0'); // nicer
      ++ptr;

      *ptr = 6; // ack
      ++ptr;
      
      snprintf(ptr, 3*n, "%x", ack_id);
      while (*ptr != 0)
        ++ptr;
      *ptr = 31; // ascii unit separator as msg separator
      ++ptr;

      auto& tba = tobeacked[sender];
      for (int i = 0; i < 8; i++) {
        snprintf(ptr, 3*n, "%x", tobeacked[sender].back());
        while (*ptr != 0)
          ++ptr;
        *ptr = 31; // ascii unit separator as msg separator
        ++ptr;
        tobeacked[sender].pop_back();
        if (tobeacked[sender].empty())
          break;
      }

      // change back last one
      --ptr;
      *ptr = 0; // ascii unit separator as msg separator
      
      pending_acks[sender][ack_count] = buffer;
    }

    void send_acks() {
      if (OO >= 2) std::cout << "running acks in stubborn" << std::endl;
      while (true) {
        if (OO >= 3) std::cout << "ack loop" << std::endl;

        // add to pending from tobeacked if haven't received more in a while
        if (ack_cycles % 50 == 0) {
          std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
          for (auto& [sender, last] : last_add_to_pending) {
            {
              std::lock_guard<std::mutex> lock(ack_mutx);
              if (tobeacked[sender].empty())
                continue;
            }
            if (now - last >= std::chrono::seconds(1))
              compose_ack(sender);
          }
        }

        {
          std::lock_guard<std::mutex> lock(ack_mutx);
          for (auto& [sender, ack_msgs] : pending_acks) {
            sockaddr_in* dest = &(*addrs)[sender];
            for (auto msg : ack_msgs) {
              s_ack++;
              fl.send(msg.second, dest);
            }
          }
        }

        ack_cycles++;
        std::unique_lock<std::mutex> lock(ack_mutx);
        cv_acks.wait_for(lock, std::chrono::milliseconds(100));
      }
    }

    void receive_ack(unsigned char sender, char* msg, char* end) {
      if (OO >= 1)
      std::cout << "ack from " << static_cast<short>(sender) << ": " << msg << std::endl;
      r_ack++;

      char* sep = msg;
      
      // COMPOSE ACKACK
      // char + char + char + ack_id + null
      // id | ack | ack | ack_id
      const unsigned char n = sizeof(unsigned int) + 1;
      char* buffer = static_cast<char*>(malloc(1 + 1 + 1 + 3*n + 1));
      char* ptr = buffer;
      
      *ptr = static_cast<char>(id + '0');
      ++ptr;
      
      *ptr = 6; // ack
      ++ptr;
      *ptr = 6; // ack
      ++ptr;
      
      unsigned int ack_id = static_cast<unsigned int>(strtoul(sep, &sep, 16));
      snprintf(ptr, 3*n, "%x", ack_id);
      sep++;

      fl.send(buffer, &(*addrs)[sender]);
      s_ackack++;

      // actually ACK

      while (sep != end) {
        unsigned int msg_id = static_cast<unsigned int>(strtoul(sep, &sep, 16));
        if (OO >= 2)
          std::cout << "removing from sending: " << msg_id << std::endl;
        // remove msg_id from sending
        {
          std::lock_guard<std::mutex> lock(Q_mutx);
          auto it = msg_index.find(msg_id);
          if (it != msg_index.end()) {
            it->second->acked = true;
            msg_index.erase(it);
          }
        }

        if (end != sep)
          sep++;
      }
    }

    void receive_ackack(unsigned char sender, char* msg, char* end) {
      r_ackack++;
      unsigned int ack_id = static_cast<unsigned int>(strtoul(msg, nullptr, 16));
      // remove from pending
      if (OO >= 1) std::cout << "ackack from " << static_cast<short>(sender) << ": " << ack_id << std::endl;
      std::lock_guard<std::mutex> lock(ack_mutx);
      auto it = pending_acks[sender].find(ack_id);
      if (it != pending_acks[sender].end()) {
        pending_acks[sender].erase(it);
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
    unsigned int ack_count = 0;
    std::unordered_set<unsigned long> lookup{};
    std::queue<StubbornMsg*> Q;
    std::unordered_map<unsigned int, StubbornMsg*> msg_index;
    std::mutex Q_mutx;
    std::mutex ack_mutx;
    std::condition_variable cv_ready;
    std::condition_variable cv_empty;
    std::condition_variable cv_acks;
    std::unordered_map<unsigned char, struct sockaddr_in>* addrs;
    std::unordered_map<unsigned char, std::vector<unsigned int>> tobeacked;
    std::unordered_map<unsigned char, std::chrono::steady_clock::time_point> last_add_to_pending;
    std::unordered_map<unsigned char, std::unordered_map<unsigned int, char*>> pending_acks;

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