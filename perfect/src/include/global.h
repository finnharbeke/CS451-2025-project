#pragma once

const unsigned char OO = 0; // verbosity 0 - nothing - 1 - important - 2 - not really - 3 - rest
const bool OOTIME = 0;

const unsigned long LOGBUFSIZE = 1 << 8; // 1/4 MiB, so for 128 proc, 32 MiB
const unsigned long LOGLINESIZE = 64; // longest possible log line

const unsigned long MAX_PENDING = 1024;

const unsigned long MSGS_2_TIME = 10000;