#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "parser.hpp"
#include "global.h"
#include <signal.h>

#include "process.cpp"
#include "config.cpp"

Process* proc;

static void stop(int) {
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  if (OO >= 1) {
    // immediately stop network packet processing
    std::cout << "Immediately stopping network packet processing.\n";

    // write/flush output file if necessary
    std::cout << "Writing output.\n";
  }

  // what happens if logging thread logs after this guy closes ofstream
  proc->close();

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

  auto hosts = parser.hosts();
  if (OO >= 1) {
    std::cout << std::endl;

    std::cout << "My PID: " << getpid() << "\n";
    std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
              << getpid() << "` to stop processing packets\n\n";

    std::cout << "My ID: " << parser.id() << "\n\n";

    std::cout << "List of resolved hosts is:\n";
    std::cout << "==========================\n";
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
  }
  
  LatticeConfig config(parser.configPath());
  
  unsigned char n = static_cast<unsigned char>(hosts.size());
  std::unordered_map<unsigned char, struct sockaddr_in> addrs;

  for (Parser::Host host : hosts) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = host.port;
    addr.sin_addr.s_addr = host.ip;
    
    addrs[static_cast<unsigned char>(host.id)] = addr;
  }

  switch (n) {
    case 1:
    case 2:
      MAX_ACTIVE_WINDOW = 50;
      INIT_BACKOFF = 4;
      RETRY_UNANSWERED_MILLIS = 150;
      break;
    case 3:
      MAX_ACTIVE_WINDOW = 40;
      INIT_BACKOFF = 4;
      RETRY_UNANSWERED_MILLIS = 150;
      break;
    case 4:
    case 5:
      MAX_ACTIVE_WINDOW = 30;
      INIT_BACKOFF = 5;
      RETRY_UNANSWERED_MILLIS = 150;
      break;
    case 6:
    case 7:
    case 8:
      MAX_ACTIVE_WINDOW = 20;
      INIT_BACKOFF = 6;
      RETRY_UNANSWERED_MILLIS = 200;
      break;
    case 9:
    case 10:
      MAX_ACTIVE_WINDOW = 15;
      INIT_BACKOFF = 6;
      RETRY_UNANSWERED_MILLIS = 250;
      break;
    default:
      MAX_ACTIVE_WINDOW = 250 / n;
      INIT_BACKOFF = 7;
      RETRY_UNANSWERED_MILLIS = 400;
  }

  // std::ios_base::sync_with_stdio(false);
  // std::cin.tie(nullptr);
  if (OO >= 1) std::cout << "Broadcasting and delivering messages...\n\n";
  proc = new Process(parser.id(), n, &config, parser.outputPath(), &addrs);
  proc->main();

  if (OO >= 1) std::cout << "All done, let's sleep!\n";

  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true) {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
