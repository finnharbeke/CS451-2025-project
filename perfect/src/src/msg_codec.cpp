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
        ptr += _ID_S; // beb id
        ptr++; // unit sep
        int written = snprintf(ptr, _TIME_S + 1, "%.16lx", timestamp);
        ptr += written;
        *ptr = 30; // ascii record separator as msg separator
        ++ptr;
    }

    void add_beb_msg_sender_n_id_n_ts(char* msg, unsigned char from, unsigned long msg_id);
    void add_beb_msg_sender_n_id_n_ts(char* msg, unsigned char from, unsigned long msg_id)
    {
        char *ptr = msg;
        *ptr = static_cast<char>(from + '0'); // nicer
        ptr++;
        int written = snprintf(ptr, _ID_S + 1, "%.16lx", msg_id);
        ptr += written;
        *ptr = 30; // record sep
        ptr++;
        add_timestamp(msg, std::chrono::system_clock::now());
    }

    char* make_heartbeat(unsigned char id);
    char* make_heartbeat(unsigned char id) {
        char* b = static_cast<char*>(malloc(HEARTBEAT_LEN));
        *b = static_cast<char>(id + '0');
        *(b+1) = '&'; // ampersand for life
        *(b+2) = '\0';
        return b;
    }

    char* compose_agreement_msg(char marker, bool with_set, unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal);
    char* compose_agreement_msg(char marker, bool with_set, unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal) {
        char* msg = static_cast<char*>(malloc(
            PACKET_HEADER_LEN + (with_set ? proposal.size() * (_CMPRSD_S+1) : 0)
        ));
        char* ptr = msg;

        ptr++; // beb_sender
        ptr += _ID_S; // beb msg_id
        ptr++; // rs
        ptr += _TIME_S; // time
        ptr++; // rs
        *ptr++ = marker; // enquiry / ack / nak
        *ptr++ = static_cast<char>(round + '0');
        ptr += snprintf(ptr, _CMPRSD_S + 1, "%x", p_id);
        if (with_set) {
            for (auto x : proposal) {
                *ptr++ = 31;
                ptr += snprintf(ptr, _CMPRSD_S + 1, "%x", x);
            }
        }
        *ptr = '\0';
        
        return msg;
    }
    
    char* compose_proposal(unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal);
    char* compose_proposal(unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal) {
        return compose_agreement_msg(5, true, p_id, round, proposal);
    }
    char* compose_ack(unsigned int p_id, unsigned char round);
    char* compose_ack(unsigned int p_id, unsigned char round) {
        std::vector<unsigned int> empty;
        return compose_agreement_msg(6, false, p_id, round, empty);
    }
    char* compose_nak(unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal);
    char* compose_nak(unsigned int p_id, unsigned char round, std::vector<unsigned int>& proposal) {
        return compose_agreement_msg(21, true, p_id, round, proposal);
    }


}