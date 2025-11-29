#pragma once

#include "global.h"

#include <cstdio>
#include <chrono>
#include <stdexcept>

namespace codec
{

    char *compose_batch(unsigned char from, unsigned int msg_id, unsigned char nr_msgs, unsigned int seq_nr);
    char *compose_batch(unsigned char from, unsigned int msg_id, unsigned char nr_msgs, unsigned int seq_nr)
    {
        if (nr_msgs > MAX_MSG_PER_PACKET)
        {
            throw std::invalid_argument("'nr_msgs' larger than 8");
        }

        char *buffer = static_cast<char *>(malloc(PACKET_LEN));
        char *ptr = buffer;

        // sender id
        *ptr = static_cast<char>(from + '0'); // nicer
        ptr++;

        int written = snprintf(ptr, _CMPRSD_S + 1, "%x", msg_id);
        ptr += written;
        *ptr = 31; // ascii unit separator as msg separator
        ptr++;

        // TIMESTAMP
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        snprintf(ptr, _TIME_S + 1, "%.16lx", timestamp);
        while (*ptr != 0)
            ++ptr;
        *ptr = 31; // ascii unit separator as msg separator
        ++ptr;

        for (unsigned char i = 0; i < nr_msgs; i++)
        {
            // write the longs as hexadecimals
            snprintf(ptr, _CMPRSD_S + 1, "%x", seq_nr + i);
            while (*ptr != 0)
                ++ptr;
            *ptr = 31; // ascii unit separator as msg separator
            ++ptr;
        }
        // change back last one
        --ptr;
        *ptr = 0;

        return buffer;
    }

}