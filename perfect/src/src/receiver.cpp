#pragma once

#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "stubborn.cpp"

class Receiver {
public:
  
  Receiver(unsigned long id, const char* outputPath,
      sockaddr_in* address,
      std::vector<std::pair<unsigned long, sockaddr_in>>* senders)
      : address(address), senders(senders) {
    std::cout << "setting up receiver with process id " << id << std::endl;
    out.open(outputPath);
    std::cout << "opened file " << outputPath << " " << out.is_open() << " " << !out << std::endl;
    network.bind_address(address);
  }

  void main() {
    while (true) {
      receive();
    }
  }
  
  void receive() {
    unsigned long from;
    char buffer[256] = {0};
    char* ptr = buffer;
    // char* msg;
    network.receive(&from, &ptr);

    std::cout << "rec " << from << " " << *ptr << std::endl; 
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
  Stubborn network;
  sockaddr_in* address;
  std::vector<std::pair<unsigned long, sockaddr_in>>* senders;
  char* outputPath;
  std::ofstream out;
};