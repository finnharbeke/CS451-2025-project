#pragma once

#include <iostream>
#include <fstream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "perfect.cpp"

class Sender {
public:
  Sender(unsigned long id_, unsigned long m, const char* outputPath, sockaddr_in* receiver)
    : receiver(receiver), m(m) {
    std::cout << "setting up sender with process id " << id << std::endl;
    out.open(outputPath);
    id = static_cast<char>(id_ & 0xFF); // by assumptions ok
    std::cout << "set up sender '" << id << "' = " << static_cast<short>(id) << std::endl;
  }

  void main() {
    for (unsigned long i = 0; i < m; i += 8) {
      send_k((m - i) < 8 ? static_cast<char>(m - i) : 8, i + 1);
    }
  }
  
  void send_k(char k, unsigned long seq_nr) {
    if (network.send_k(id, k, seq_nr, receiver)) {
      std::cout << static_cast<short>(id) << " sent 8 from " << seq_nr << std::endl;
      log(seq_nr); // TODO
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  void log(unsigned long msg) {
    out << "b " << msg << "\n";
  }

  void close() {
    out.close();
  }

private:
  Perfect network;
  sockaddr_in* receiver;
  char id;
  unsigned long m;
  std::ofstream out;
};