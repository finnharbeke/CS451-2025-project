#pragma once

#include <stdlib.h>

// verbosity
// 0 - nothing
// 1 - non-msg_triggered &  std::cerr
// 2 - crucial msg triggers: stubborn receives & sends
// 3 - other msg triggers
// 4 - msg contents
const unsigned char OO = 0;
const bool STATS = 1;

const unsigned long LOGBUFSIZE = 1 << 18; // 1/4 MiB, so for 128 proc, 32 MiB
const unsigned long LOG_CAP = LOGBUFSIZE - (1 << 8); // when to flush
const unsigned long RCVBUFSIZE = 1 << 14; // 16 KiB, so for 128 proc, ...
const unsigned long MSGQSIZE = 1 << 10; // 1024 msgs in queue at a time

const unsigned long MAX_PENDING = 1024; // messages in queue at a time
const unsigned long REFILL = 768; // when to start sending again
const unsigned long SEND_BURST = MAX_PENDING - REFILL;
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

// PACKET
// id msg_id | timepoint | seq_nr | seq_nr | seq_nr null
const size_t PACKET_LEN = 1 + _CMPRSD_S + 1 + _TIME_S + (1+_CMPRSD_S) * MAX_MSG_PER_PACKET + 1;
// ACK
// id ack left | right null
const size_t ACK_LEN = 2 + _CMPRSD_S + 1 + _CMPRSD_S + 1;
// ACKACK
// id ack ack timepoint | left | right null
const size_t ACKACK_LEN = 3 + _TIME_S + 1 + _CMPRSD_S + 1 + _CMPRSD_S + 1;
const long MAX_RECVD = static_cast<long>(PACKET_LEN);