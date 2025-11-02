#pragma once

#include <iostream>
#include <thread>
#include <unordered_map>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "perfect.cpp"
#include "log_queue.cpp"

class Process {
public:
  Process(unsigned long id_, unsigned long dest_id, unsigned long m_, const char* outputPath,
    std::unordered_map<unsigned char, struct sockaddr_in>* addrs_)
    : addrs(addrs_), id(static_cast<unsigned char>(id_)), network(Perfect(id, addrs)) {

    if (OO >= 1) std::cout << "setting up process with id " << id_ << std::endl;
    
    dest = static_cast<unsigned char>(dest_id); // by assumptions at most 128
    m = static_cast<unsigned int>(m_); // by assumptions at most 2^31-1
    network.bind_address(&(*addrs)[id]);
    queue = new LogQueue(outputPath);

    if (OO >= 1) std::cout << "set up process " << static_cast<short>(id) << std::endl;
  }

  void main() {
    queue->start_logging();

    auto send = std::bind(&Process::send_batch, this);
    auto receive = std::bind(&Process::log_receive, this,
      std::placeholders::_1, std::placeholders::_2);
    network.run(send, receive);
  }
  
  void close() {
    if (OO >= 1) std::cout << "closing" << std::endl;
    queue->close();
  }

  void log_receive(unsigned char sender, char* msg) {
    Log log(sender, msg);
    queue->push(&log);
  }

  bool send_batch() {
    if (seq_nr > m)
      return true;
    if (OO >= 3) std::cout << "keeping sending" << std::endl;
    // only send (MAX_PENDING >> 1) new messages
    for (unsigned int i = 0; i < 512; i++) {
      if (OO >= 3) std::cout << "send loop" << std::endl;
      
      unsigned char k;
      unsigned int d = m - seq_nr + 1;
      k = (d < 8) ? static_cast<char>(d) : 8;
      
      char* msg = compose_batch(msg_count, k, seq_nr);
      if (OO >= 2) std::cout << "composed " << msg << std::endl;
      network.send(msg_count++, msg, &(*addrs)[dest]);

      for (unsigned char j = 0; j < k; j++)
        log_send(seq_nr++);
      
      if (seq_nr > m)
        break;
      if (OO >= 3) std::cout << "d " << d << " k " << k << " seq_nr " << seq_nr << " m " << m << std::endl;
    }
    if (OO >= 1 && seq_nr > m) 
      std::cout << "process " << id << " done sending." << std::endl;
    return false;
  }

  void log_send(unsigned int seq_nr) {
    Log log(seq_nr);
    queue->push(&log);
  }

  char* compose_batch(unsigned int msg_id, unsigned char nr_msgs, unsigned int seq_nr) {
    if (nr_msgs > 8) {
      throw std::invalid_argument("'nr_msgs' larger than 8");
    }

    // char + int + (at most) 8 * (hex_int) + null
    // id  | msg_id | content us content us content | null
    const unsigned char n = sizeof(unsigned int) + 1;
    char* buffer = static_cast<char*>(malloc(1 + n + 1 + 8 * 3*n + 1));
    char* ptr = buffer;

    // sender id
    *ptr = static_cast<char>(id + '0'); // nicer
    ++ptr;

    snprintf(ptr, 3*n, "%x", msg_id);
    while (*ptr != 0)
    ++ptr;
    *ptr = 31; // ascii unit separator as msg separator
    ++ptr;
    
    for (unsigned char i = 0; i < nr_msgs; i++) {
      // write the longs as hexadecimals
      snprintf(ptr, 3*n, "%x", seq_nr + i);
      while (*ptr != 0)
        ++ptr;
      *ptr = 31; // ascii unit separator as msg separator
      ++ptr;
    }
    // change back last one
    --ptr;
    *ptr = 0; // ascii unit separator as msg separator
    
    return buffer;
  }

private:
  std::unordered_map<unsigned char, struct sockaddr_in>* addrs;
  unsigned char id;
  Perfect network;
  unsigned char dest;
  unsigned int m = 0;
  unsigned int seq_nr = 1;
  unsigned int msg_count = 1;
  LogQueue* queue;
};