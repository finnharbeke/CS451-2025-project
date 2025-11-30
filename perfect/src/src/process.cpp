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
#include "beb.cpp"
#include "ram.cpp"
#include "msg_codec.cpp"

class Process
{
public:
  Process(unsigned long id_, unsigned char n, unsigned long m_, const char *outputPath,
          std::unordered_map<unsigned char, struct sockaddr_in> *addrs_)
      : addrs(addrs_), id(static_cast<unsigned char>(id_)),
      network(BEB(id, n, addrs,
        std::bind(&Process::send_batch, this),
        std::bind(&Process::log_receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
      ))
  {

    if (OO >= 1)
      std::cout << "setting up process with id " << id_ << std::endl;

    m = static_cast<unsigned int>(m_);          // by assumptions at most 2^31-1
    network.bind_address(&(*addrs)[id]);

    // logs
    out.open(outputPath);
    send_buffer.reserve(LOGBUFSIZE);
    recv_buffer.reserve(LOGBUFSIZE);

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
    {
      std::lock_guard lock(outmutx);
      out << send_buffer << recv_buffer;
      out.flush();
      out.close();
    }
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
              << msg_count - last_msg_count << ","
              << seq_nr - last_seq_nr << ","
              << log_r - last_log_r << ",";

    last_seq_nr = seq_nr;
    last_msg_count = msg_count;
    last_log_r = log_r;
  }

  void log_receive(unsigned char from, char *msg, char* end)
  {
    // receives message formatted as
    // seq_nr | seq_nr | seq_nr ...
    auto seq_nrs = codec::recover_seqnrs(msg, end);
    for (auto seq_nr : seq_nrs) {
      if (OO >= 3)
        std::cout << "logging " << seq_nr << " from " << static_cast<short>(from) << std::endl;
      recv_buffer += "d ";
      recv_buffer += std::to_string(static_cast<short>(from));
      recv_buffer += " ";
      recv_buffer += std::to_string(seq_nr);
      recv_buffer += "\n";
      log_r++;
    }
    check_recv_buffer();
  }

  bool send_batch()
  {
    if (seq_nr > m)
      return true;
    if (OO >= 2)
      std::cout << "keeping sending from" << seq_nr << std::endl;
    // only send (MAX_PENDING >> 1) new messages
    for (unsigned int i = 0; i < SEND_BURST; i++)
    {

      unsigned char k;
      unsigned int d = m - seq_nr + 1;
      k = (d < MAX_MSG_PER_PACKET) ? static_cast<char>(d) : MAX_MSG_PER_PACKET;

      char *msg = codec::compose_batch(id, msg_count, k, seq_nr);
      if (OO >= 4)
        std::cout << "composed " << msg << std::endl;
      network.send(msg_count++, msg);

      for (unsigned char j = 0; j < k; j++)
      {
        if (OO >= 3)
          std::cout << "logging " << static_cast<short>(seq_nr) << std::endl;
        send_buffer += "b ";
        send_buffer += std::to_string(seq_nr);
        send_buffer += "\n";
        seq_nr++;
      }
      check_send_buffer();

      if (seq_nr > m)
        break;
    }
    if (OO >= 2)
      std::cout << "sent until" << seq_nr - 1 << std::endl;
    if (OO >= 1 && seq_nr > m)
      std::cout << "process " << static_cast<short>(id) << " done sending." << std::endl;
    return seq_nr > m;
  }

  void check_send_buffer()
  {
    if (send_buffer.size() >= LOG_CAP) {
      {
        std::lock_guard lock(outmutx);
        out << send_buffer;
      }
      send_buffer.clear();
    }
  }
  void check_recv_buffer()
  {
    if (recv_buffer.size() >= LOG_CAP) {
      {
        std::lock_guard lock(outmutx);
        out << recv_buffer;
      }
      recv_buffer.clear();
    }
  }

private:
  std::unordered_map<unsigned char, struct sockaddr_in> *addrs;
  unsigned char id;
  BEB network;
  unsigned int m = 0;
  unsigned int seq_nr = 1;
  unsigned int msg_count = 1;
  unsigned int log_r = 0;
  std::string send_buffer;
  std::string recv_buffer;
  std::mutex outmutx;
  std::ofstream out;

  const std::clock_t c_start = std::clock();
  const std::chrono::time_point<std::chrono::high_resolution_clock> t_start = std::chrono::high_resolution_clock::now();
  unsigned int last_seq_nr = seq_nr;
  unsigned int last_log_r = log_r;
  unsigned int last_msg_count = msg_count;
  unsigned int stats_round = 0;
};