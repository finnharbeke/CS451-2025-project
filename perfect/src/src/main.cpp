#include <chrono>
#include <iostream>
#include <thread>

#include "parser.hpp"
#include "hello.h"
#include <signal.h>

#include "sender.cpp"
#include "receiver.cpp"
#include "config.cpp"
#include "fairloss.cpp"

bool is_receiver;
Receiver* receiver;
Sender* sender;

static void stop(int) {
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  if (is_receiver)
    receiver->close();
  else
    sender->close();

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv);
  parser.parse();

  hello();
  std::cout << std::endl;

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts) {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n\n";

  std::cout << "Path to config:\n";
  std::cout << "===============\n";
  std::cout << parser.configPath() << "\n\n";

  std::cout << "Doing some initialization...\n\n";

  PerfectConfig config(parser.configPath());
  
  struct sockaddr_in rec_addr;
  std::vector<std::pair<unsigned long, sockaddr_in>> senders;
  for (Parser::Host host : hosts) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = host.port;
    addr.sin_addr.s_addr = host.ip;
    
    if (host.id == config.i) {
      rec_addr = addr;
    } else {
      senders.push_back(std::pair(host.id, addr));
    }
  }

  if (parser.id() == config.i) {
    is_receiver = true;
    std::cout << "i'm receiver\n";
    receiver = new Receiver(parser.id(), parser.outputPath(), &rec_addr, &senders);
    std::cout << "Broadcasting and delivering messages...\n\n";
    receiver->main();

  } else {
    is_receiver = false;
    sender = new Sender(parser.id(), config.m, parser.outputPath(), &rec_addr);
    std::cout << "Broadcasting and delivering messages...\n\n";
    sender->main();
  }

  std::cout << "All done, let's sleep!\n";

  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
