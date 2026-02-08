#pragma once

#include <thread>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>

// logging
#include <iostream>
#include <fstream>
#include <ios>
#include <iomanip>


#include "global.h"
#include "lattice.cpp"
#include "config.cpp"
#include "ram.cpp"
#include "msg_codec.cpp"

class Process
{
public:
  Process(unsigned long id_, unsigned char n, LatticeConfig* config, const char *outputPath,
          std::unordered_map<unsigned char, struct sockaddr_in> *addrs_)
      : addrs(addrs_), id(static_cast<unsigned char>(id_)),
        network(LatticeAgreement(id, n, config, addrs,
          std::bind(&Process::decide, this, std::placeholders::_1, std::placeholders::_2)
        )
    )
  {

    if (OO >= 1)
      std::cout << "setting up process with id " << id_ << std::endl;

    network.bind_address(&(*addrs)[id]);

    // logs
    out.open(outputPath);
    buffer.reserve(LOGBUFSIZE);

    if (OO >= 1)
      std::cout << "set up process " << static_cast<short>(id) << std::endl;
  }

  void main()
  {
    std::thread ew([&]{ network.enqueueWorker(); });
    ew.detach();
    std::thread sw([&]{ network.sendWorker(); });
    sw.detach();
    std::thread aw([&]{ network.ackWorker(); });
    aw.detach();
    std::thread rw([&]{ network.receiveWorker(); });
    rw.detach();
    std::thread l([&]{ network.listen(); });
    l.detach();
    std::thread hb([&]{ network.heartbeats(); });
    hb.detach();

    if (STATS)
    {
      std::thread statsing(&Process::keep_stats, this);
      statsing.detach();
    }
  }

  void close()
  {
    if (OO >= 1)
      std::cout << "closing" << std::endl;
    out << buffer;
    out.flush();
    out.close();
  }

  void keep_stats()
  {
    // output some time and other measurements
    while (true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(ST_INTERVAL_MILLIS));

      stats(); // P-id,CPU,WC,RAM,m_count,seq_id
      // PF  sent,received
      // STB sent,s_cycles,ack_s,ack_r,ack_cyc,recv,recvtot,aas,aar
      // FL  sent,recv
      network.stats();
    }
  }

  void stats()
  {
    const std::clock_t c_end = std::clock();
    const std::chrono::time_point<std::chrono::high_resolution_clock> t_end = std::chrono::high_resolution_clock::now();
    std::cout << static_cast<short>(id) << "," << stats_round++ << ","
              << std::fixed << std::setprecision(0)
              << 1000.0 * static_cast<double>(c_end - c_start) / CLOCKS_PER_SEC << ","
              << std::chrono::duration<double, std::milli>(t_end - t_start).count() << ","
              << getCurrentRAM() << ","
              << logged - last_logged << ",";

    last_logged = logged;
  }

  void decide(unsigned int slot, std::vector<unsigned int>& value)
  {
    if (OO >= 2)
      std::cout << "logging decision " << slot << std::endl;
    auto it = value.begin();
    const auto end = value.end();
    if (it != end)
      buffer += std::to_string(*it++);
    while (it != end) {
      buffer += " " + std::to_string(*it++);
    }
    buffer += '\n';
    decs_in_buf++;
    check_buffer();
    logged++;
  }

  void check_buffer()
  {
    if (decs_in_buf >= MAX_DECS_IN_BUF || buffer.size() >= LOG_CAP) {
      out << buffer;
      buffer.clear();
      decs_in_buf = 0;
    }
  }

private:
  std::unordered_map<unsigned char, struct sockaddr_in> *addrs;
  unsigned char id;
  LatticeAgreement network;
  unsigned int logged = 0;
  std::string buffer;
  std::ofstream out;
  unsigned int decs_in_buf = 0;

  const std::clock_t c_start = std::clock();
  const std::chrono::time_point<std::chrono::high_resolution_clock> t_start = std::chrono::high_resolution_clock::now();
  unsigned int last_logged = logged;
  unsigned int stats_round = 0;
};