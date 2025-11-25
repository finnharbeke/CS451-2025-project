#pragma once

#include <iostream>
#include <thread>
#include <unordered_map>
#include <iomanip>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "perfect.cpp"
#include "log_queue.cpp"
#include "ram.cpp"

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
    if (STATS) {
      std::thread statsing(&Process::keep_stats, this);
      statsing.detach();
    }
  }
  
  void close() {
    if (OO >= 1) std::cout << "closing" << std::endl;
    queue->close();
  }
  
  void keep_stats() {
    // output some time and other measurements
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(ST_INTERVAL_MILLIS));

      stats(); // P-id,CPU,WC,RAM,m_count,seq_id
      queue->stats(); // n_logged
      // PF  sent,received
      // STB sent,s_cycles,ack_s,ack_r,ack_cyc,recv,recvtot,aas,aar
      // FL  sent,recv
      network.stats();
    }
  }

  void stats() {
    const std::clock_t c_end = std::clock();
    const std::chrono::time_point<std::chrono::high_resolution_clock> t_end = std::chrono::high_resolution_clock::now();
    std::cout << static_cast<short>(id) << "," << stats_round++ << ","
              << std::fixed << std::setprecision(0)
              << 1000.0 * static_cast<double>(c_end - c_start) / CLOCKS_PER_SEC << ","
              << std::chrono::duration<double, std::milli>(t_end - t_start).count() << ","
              << getCurrentRAM() << ","
              << msg_count - last_msg_count << ","
              << seq_nr - last_seq_nr << ",";

    last_seq_nr = seq_nr;
    last_msg_count = msg_count;
  }

  void log_receive(unsigned char sender, char* msg) {
    Log log(sender, msg);
    queue->push(&log);
  }

  bool send_batch() {
    if (seq_nr > m)
      return true;
    if (OO >= 2) std::cout << "keeping sending from" << seq_nr << std::endl;
    // only send (MAX_PENDING >> 1) new messages
    for (unsigned int i = 0; i < SEND_BURST; i++) {
      
      unsigned char k;
      unsigned int d = m - seq_nr + 1;
      k = (d < MAX_MSG_PER_PACKET) ? static_cast<char>(d) : MAX_MSG_PER_PACKET;
      
      char* msg = compose_batch(msg_count, k, seq_nr);
      if (OO >= 4) std::cout << "composed " << msg << std::endl;
      network.send(msg_count++, msg, &(*addrs)[dest]);

      for (unsigned char j = 0; j < k; j++)
        log_send(seq_nr++);
      
      if (seq_nr > m)
        break;
    }
    if (OO >= 2) std::cout << "sent until" << seq_nr-1 << std::endl;
    if (OO >= 1 && seq_nr > m) 
      std::cout << "process " << static_cast<short>(id) << " done sending." << std::endl;
    return false;
  }

  void log_send(unsigned int seq_nr) {
    Log log(seq_nr);
    queue->push(&log);
  }

  char* compose_batch(unsigned int msg_id, unsigned char nr_msgs, unsigned int seq_nr) {
    if (nr_msgs > MAX_MSG_PER_PACKET) {
      throw std::invalid_argument("'nr_msgs' larger than 8");
    }

    char* buffer = static_cast<char*>(malloc(PACKET_LEN));
    char* ptr = buffer;

    // sender id
    *ptr = static_cast<char>(id + '0'); // nicer
    ++ptr;

    snprintf(ptr, _CMPRSD_S, "%x", msg_id);
    while (*ptr != 0)
    ++ptr;
    *ptr = 31; // ascii unit separator as msg separator
    ++ptr;

    // TIMESTAMP
    auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned int front = static_cast<unsigned int>(timestamp >> 32);
    unsigned int back = static_cast<unsigned int>(timestamp & UINT32_MAX);
    snprintf(ptr, _CMPRSD_S, "%x", front);
    while (*ptr != 0)
    ++ptr;
    snprintf(ptr, _CMPRSD_S, "%x", back);
    while (*ptr != 0)
    ++ptr;
    *ptr = 31; // ascii unit separator as msg separator
    ++ptr;
    
    for (unsigned char i = 0; i < nr_msgs; i++) {
      // write the longs as hexadecimals
      snprintf(ptr, _CMPRSD_S, "%x", seq_nr + i);
      while (*ptr != 0)
        ++ptr;
      *ptr = 31; // ascii unit separator as msg separator
      ++ptr;
    }
    // change back last one
    --ptr;
    *ptr = 0;
    
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
  
  const std::clock_t c_start = std::clock();
  const std::chrono::time_point<std::chrono::high_resolution_clock> t_start = std::chrono::high_resolution_clock::now();
  unsigned int last_seq_nr = seq_nr;
  unsigned int last_msg_count = msg_count;
  unsigned int stats_round = 0;
};