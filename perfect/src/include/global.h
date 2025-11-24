#pragma once

const unsigned char OO = 0; // verbosity 0 - nothing - 1 - important - 2 - not really - 3 - rest
const bool STATS = 1;

const unsigned long LOGBUFSIZE = 1 << 8; // 1/4 MiB, so for 128 proc, 32 MiB
const unsigned long LOGLINESIZE = 64; // longest possible log line

const unsigned long MAX_PENDING = 1024;

// stats stuff
const long ST_INTERVAL_MILLIS = 1000;