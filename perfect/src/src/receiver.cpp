#pragma once

#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "perfect.cpp"

class Receiver {
public:
  
  Receiver(unsigned long id_, const char* outputPath,
      sockaddr_in* address) : address(address) {
    if (OO) std::cout << "setting up receiver with process id " << id << std::endl;
    
    id = static_cast<char>(id_ & 0xFF); // by assumptions at most 128
    out.open(outputPath);
    network.bind_address(address);

    if (OO) std::cout << "set up receiver " << static_cast<short>(id) << std::endl;
  }

  void main() {
    while (true) {
      receive();
    }
  }
  
  void receive() {
    unsigned char from;
    char buffer[256] = {0};
    char* ptr = buffer;
    // char* msg;
    network.receive(&from, &ptr);

    std::cout << "rec " << from << " " << ptr << std::endl; 
    log(from, ptr);
  }

  void log(unsigned long sender, char* msg) {
    out << "d " << sender << " " << msg << "\n";
  }

  void close() {
    std::cout << "closing" << std::endl;
    out.close();
  }

private:
  Perfect network;
  sockaddr_in* address;
  char id;
  char* outputPath;
  std::ofstream out;
};