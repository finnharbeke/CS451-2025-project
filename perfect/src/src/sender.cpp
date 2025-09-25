#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include "parser.hpp"

class Sender {
public:
  
  Sender(long unsigned int id, int m, sockaddr_in receiver) {
    std::cout << "setting up sender with process id " << id << std::endl;

    // make a socket:

    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("couldn't create socket for sender\n");
    }

    const char *msg = "my first message!!";
    ssize_t bytes_sent;

    bytes_sent = sendto(sock, msg, strlen(msg), 0, reinterpret_cast<sockaddr*>(&receiver), sizeof(receiver));

    if (bytes_sent < 0) {
      std::cout << "couldn't send, errno " << errno << ", " << strerror(errno) << std::endl;
    } else {
      std::cout << "sent " << bytes_sent << " bytes!!" << std::endl;
    }
  }

private:
  int sock;

};