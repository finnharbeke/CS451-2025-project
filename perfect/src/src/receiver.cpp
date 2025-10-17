#pragma once

#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"
#include "fairloss.cpp"

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
      sockaddr_in from;
      char buffer[1024] = {0};
      network.receive(&from, buffer);
      parse(&from, buffer);
    }
  }

  void parse(sockaddr_in* from, char* buffer) {
    unsigned long sender_id = 0;
    for (std::pair<unsigned long, sockaddr_in> sender : *senders) {
      if (sender.second.sin_port == from->sin_port) {
        sender_id = sender.first;
        break;
      }
    }
    log(sender_id, buffer);
  }

  void log(unsigned long sender, char* buffer) {
    out << "d " << sender << " " << buffer << "\n";
  }

  void close() {
    std::cout << "closing" << std::endl;
    out.close();
  }

private:
  FairLoss network;
  sockaddr_in* address;
  std::vector<std::pair<unsigned long, sockaddr_in>>* senders;
  char* outputPath;
  std::ofstream out;
};