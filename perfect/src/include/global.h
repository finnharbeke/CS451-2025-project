#pragma once

#include <stdlib.h>
#include <atomic>

// verbosity
// 0 - nothing
// 1 - setup/close & heartbeats & std::cerr
// 2 - agreement things
// 3 - ack things
// 4 - crucial msg triggers: stubborn receives & sends
// 5 - other msg triggers
// 6 - msg contents
const unsigned char OO = 0;
const bool STATS = 1;

const unsigned short MAX_DS = 1000;
const unsigned short MAX_VS = 100;
const unsigned int SLOTS_AHEAD = 10;
const unsigned int MAX_ACTIVE_WINDOW = 20;

const unsigned long LOGBUFSIZE = 1 << 18; // 1/4 MiB, so for 128 proc, 32 MiB
const unsigned long LOGLINE_MAXLEN = MAX_DS * 11; // 2^32-1 is 10 digits, plus space
const unsigned long LOG_CAP = LOGBUFSIZE - LOGLINE_MAXLEN; // when to flush
const unsigned long RCVBUFSIZE = 1 << 15; // 32 KiB, so for 128 proc, 2MiB
const unsigned long MSGQSIZE = 1 << 10; // 1024 msgs in queue at a time

const unsigned long MAX_PENDING = 2048; // messages in queue at a time
const unsigned long REFILL = 768; // when to start sending again
const unsigned long SEND_BURST = 256;
const unsigned char INIT_BACKOFF = 4; // 2^4 ms init time to back off
const unsigned char MAX_BACKOFF = 10; // 2^10 ms ~1s max time to back off
const unsigned long ACK_INTERVAL_MILLIS = 100;

// stats stuff
const long ST_INTERVAL_MILLIS = 1000;

// PACKET SIZES
const char MAX_MSG_PER_PACKET = 8;
const unsigned char _TIME_S = 16; // long long in hex
const unsigned char _INT_S = sizeof(unsigned int); // in bytes, usually 4 ie max is 2^32-1
const unsigned char _CMPRSD_S = 8; // space for hexadecimal to write unsigned int
const unsigned char _ID_S = 16; // beb_msgids can be more than urb, so more than int holds

// PACKET
// id bebmsg_id | timepoint | orig_id urbmsg_id | seq_nr | seq_nr | seq_nr null
// const size_t PACKET_LEN = 1 + _ID_S + 1 + _TIME_S + 1 + 1 + _CMPRSD_S + (1+_CMPRSD_S) * MAX_MSG_PER_PACKET + 1;
// | is rs (30)
// ; is us (31)
// sender bebmsg_id | timepoint | ack round p_id null 
// sender bebmsg_id | timepoint | nak round p_id;x;y;z;a | ack round null 
// sender bebmsg_id | timepoint | prop round p_id;a;b;c | prop round p_id;b;f;g null
const size_t PACKET_HEADER_LEN = 1 + _ID_S + 1 + _TIME_S + 1 + 2 + _CMPRSD_S + 1; // len of lattice ACK
const size_t LONGEST_PACKET_LEN = PACKET_HEADER_LEN + (1+_CMPRSD_S) * MAX_DS;

const size_t PAYLOAD_LEN = (_CMPRSD_S+1) * MAX_MSG_PER_PACKET;
// ACK
// id ack left | right null
const size_t ACK_LEN = 2 + _ID_S + 1 + _ID_S + 1;
// ACKACK
// id ack ack timepoint | left | right null
const size_t ACKACK_LEN = 3 + _TIME_S + 1 + _ID_S + 1 + _ID_S + 1;
// heartbeat
// id '&' null
const size_t HEARTBEAT_LEN = 3;
const long SUSPECT = 12; // 1.2s which should really be enough by the 1s guarantee and many attempts.
const long HEARTBEAT_MS = 100;
const long MAX_RECVD = static_cast<long>(LONGEST_PACKET_LEN);

// std::atomic<bool> resumed_flag{false};

