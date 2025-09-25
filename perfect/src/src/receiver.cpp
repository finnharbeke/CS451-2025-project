#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"

class Receiver {
public:
  
  Receiver(long unsigned int id, int m, struct sockaddr_in address, std::vector<struct sockaddr_in> senders) {
    std::cout << "setting up receiver with process id " << id << std::endl;

    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("couldn't create socket for receiver\n");
      exit(-1);
    }
    
    if (bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
      perror("couldn't bind socket of receiver\n");
      close(sock);
      exit(-1);
    }

    auto sender = senders.at(0);
    unsigned int sender_len = sizeof(sender);

    char buff[1024] = {0};
    ssize_t msg_len = recvfrom(sock, buff, 1024, 0, reinterpret_cast<sockaddr*>(&sender), &sender_len);
    if (msg_len < 0) {
      perror("reading error...\n");
      close(sock);
      exit(-1);
    }

    std::cout.write(buff, msg_len);
    std::cout << "eee i read!" << std::endl;

    close(sock);
  }

private:
  int sock;

};