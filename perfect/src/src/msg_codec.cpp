#pragma once

#include "global.h"

#include <cstdio>
#include <chrono>
#include <stdexcept>
#include <vector>

namespace codec
{
    void add_timestamp(char* msg, std::chrono::system_clock::time_point when);
    void add_timestamp(char* msg, std::chrono::system_clock::time_point when) {
        // TIMESTAMP
        auto timestamp = when.time_since_epoch().count();
        char* ptr = msg;
        ptr++; // beb sender
        ptr += _ID_S; // beb sender
        ptr++; // unit sep
        int written = snprintf(ptr, _TIME_S + 1, "%.16lx", timestamp);
        ptr += written;
        *ptr = 31; // ascii unit separator as msg separator
        ++ptr;
    }

    char *compose_batch(unsigned char from, unsigned char nr_msgs, unsigned int seq_nr);
    char *compose_batch(unsigned char from, unsigned char nr_msgs, unsigned int seq_nr)
    {
        if (nr_msgs > MAX_MSG_PER_PACKET)
        {
            throw std::invalid_argument("'nr_msgs' larger than 8");
        }

        char *buffer = static_cast<char *>(malloc(PACKET_LEN));
        char *ptr = buffer;

        // leave sender id empty
        // *ptr = static_cast<char>(from + '0'); // nicer
        ptr++;
        
        // leave beb_id empty
        ptr += _ID_S;
        *ptr = 31; // ascii unit separator as msg separator
        ptr++;

        auto now = std::chrono::system_clock::now();
        add_timestamp(buffer, now);
        ptr += _TIME_S;
        ptr++; // unit sep
        
        // original sender id
        ptr++;

        // leave urbid empty
        ptr += _CMPRSD_S;
        *ptr = 31; // ascii unit separator as msg separator
        ptr++;

        for (unsigned char i = 0; i < nr_msgs; i++)
        {
            // write the longs as hexadecimals
            int written = snprintf(ptr, _CMPRSD_S + 1, "%x", seq_nr + i);
            ptr += written;
            *ptr = 31; // ascii unit separator as msg separator
            ++ptr;
        }
        // change back last one
        --ptr;
        *ptr = 0;

        return buffer;
    }

    void add_beb_msg_sender_n_id(char* msg, unsigned char from, unsigned long msg_id);
    void add_beb_msg_sender_n_id(char* msg, unsigned char from, unsigned long msg_id)
    {
        char *ptr = msg;
        *ptr = static_cast<char>(from + '0'); // nicer
        ptr++;
        int written = snprintf(ptr, _ID_S + 1, "%.16lx", msg_id);
        ptr += written;
        *ptr = 31; // unit sep
        ptr++;
    }
    
    void add_urb_msg_sender_n_id(char* msg, unsigned char from, unsigned int msg_id);
    void add_urb_msg_sender_n_id(char* msg, unsigned char from, unsigned int msg_id)
    {
        char *ptr = msg;
        ptr++; // sender_id
        ptr += _ID_S; // beb_id
        ptr++; // unit sep
        ptr += _TIME_S; // timestamp
        ptr++; // unit sep
        *ptr = static_cast<char>(from + '0'); // orig sender_id
        ptr++;
        int written = snprintf(ptr, _CMPRSD_S + 1, "%.8x", msg_id);
        ptr += written;
        *ptr = 31; // unit sep
        ptr++;
    }

    char* beb_from_other(char* urb_part);
    char* beb_from_other(char* urb_part) {
        char *buffer = static_cast<char *>(malloc(PACKET_LEN));
        char *ptr = buffer;

        // leave sender id empty
        // *ptr = static_cast<char>(from + '0'); // nicer
        ptr++;
        
        // leave beb_id empty
        ptr += _ID_S;
        *ptr = 31; // ascii unit separator as msg separator
        ptr++;

        auto now = std::chrono::system_clock::now();
        add_timestamp(buffer, now);
        ptr += _TIME_S;
        ptr++; // unit sep
        
        strcpy(ptr, urb_part);
        return buffer;
    }

    std::vector<unsigned int> recover_seqnrs(char* msg, char* end);
    std::vector<unsigned int> recover_seqnrs(char* msg, char* end) {
        std::vector<unsigned int> v;
        char* sep = msg;
        if (OO >= 4)
            std::cout << "buffer (size " << (end-msg) << ") " << msg << std::endl;
        while (sep != end) {
            if (OO >= 4)
            std::cout << "rest buffer " << sep << std::endl;
            
            unsigned int seq_nr = static_cast<unsigned int>(strtoul(sep, nullptr, 16));
            v.push_back(seq_nr);
            
            sep = std::find(sep, end, static_cast<char>(31));
            // end sub_msg (instead of unit separator 31)
            if (end != sep) {
                *sep = '\0';
                sep++;
            }
        }
        return v;
    }

    char* make_heartbeat(unsigned char id);
    char* make_heartbeat(unsigned char id) {
        char* b = static_cast<char*>(malloc(HEARTBEAT_LEN));
        *b = static_cast<char>(id + '0');
        *(b+1) = '&'; // ampersand for life
        *(b+2) = '\0';
        return b;
    }

}