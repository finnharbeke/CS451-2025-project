#pragma once

#include <iostream>
#include <fstream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"
#include "perfect.cpp"
#include "log_queue.cpp"

class Sender {
public:
  Sender(unsigned long id_, unsigned long m_, const char* outputPath, sockaddr_in* receiver)
    : receiver(receiver) {
    if (OO) std::cout << "setting up sender with process id " << id_ << std::endl;
    
    id = static_cast<char>(id_+ '0'); // by assumptions at most 128
    m = static_cast<char>(m_); // by assumptions at most 2^31-1
    out.open(outputPath);

    if (OO) std::cout << "set up sender " << id << std::endl;
  }

  void main() {
    std::thread keep(&Sender::keep_sending, this);
    keep.detach();
    std::thread run(&Sender::run_network, this);
    run.detach();
    std::thread logging(&Sender::keep_logging, this);
    if (OO) std::cout << "detaching logger" << std::endl;
    logging.detach();

  }
  
  void close() {
    out.close();
  }

  void keep_sending() {
    // TODO
    // only send 512 new messages (arbitrary)
    // if at most 512 still being sent by network
    if (OO) std::cout << "keeping sending" << std::endl;
    for (unsigned int i = 0; i < 512; i++) {
      unsigned char k;
      unsigned int d = m - seq_nr;
      if (d < 7)
        k = static_cast<char>(d + 1);
      else
        k = 8;
      
      char* msg = compose_batch(k, seq_nr);
      if (OO) std::cout << "composed " << msg << std::endl;
      if (network.send(msg, receiver)) {
        for (unsigned char j = 0; j < k; j++) {
          SendLog log(seq_nr + j);
          queue.push(&log);
        }
      }
      seq_nr += k;
      if (seq_nr > m)
        break;
      if (CHILL) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

  }

  void keep_logging() {
    while (true) {
      SendLog log;
      
      if (queue.pop(&log)) {
        LogQueue<SendLog>::log(&out, &log);
      }
    }
  }

  void run_network() {}

  char* compose_batch(unsigned char nr_msgs, unsigned int seq_nr) {
    if (nr_msgs > 8) {
      throw std::invalid_argument("'nr_msgs' larger than 8");
    }

    // char + int + (at most) 8 * (hex_int) + null
    // id  | msg_id | content us content us content | null
    const unsigned char n = sizeof(unsigned int);
    char* buffer = static_cast<char*>(malloc(1 + n + 1 + 8 * 2*n + 1));
    char* ptr = buffer;

    // sender id
    *ptr = id;
    ++ptr;

    snprintf(ptr, 2*n, "%x", msg_id);
    ++msg_id;
    while (*ptr != 0)
    ++ptr;
    *ptr = 31; // ascii unit separator as msg separator
    ++ptr;
    
    for (char i = 0; i < nr_msgs; i++) {
      // write the longs as hexadecimals
      snprintf(ptr, 2*n, "%x", seq_nr + i);
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
  Perfect network;
  sockaddr_in* receiver;
  char id;
  unsigned int m = 0;
  unsigned int seq_nr = 1;
  unsigned int msg_id = 1;
  std::ofstream out;
  LogQueue<SendLog> queue;
};