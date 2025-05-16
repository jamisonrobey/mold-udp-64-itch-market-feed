#pragma once
#include <chrono>

#include "jam_utils/fd.h"
#include "jam_utils/memory_map.h"
#include <netinet/in.h>

#include "Mold_UDP_64.h"
// moldudp64 itch feed portion (multicast)

class Market_Feed {
public:
    Market_Feed(
        const std::string& path_to_itch_file,
        const std::string& multicast_group, uint16_t port, uint8_t ttl,
        std::chrono::microseconds throttle_us);

    Market_Feed() = delete;

    void start();

private:
    jam_utils::FD m_sock;
    sockaddr_in m_multi_addr{};
    std::chrono::microseconds m_throttle_us;

    jam_utils::FD m_itch_file;
    std::size_t m_file_size;
    jam_utils::Memory_Map m_mapped_file;

    static constexpr size_t LEN_PREFIX_SIZE = 2; // file composed of 2 byte len then a message
    static constexpr size_t MTU_MAX = 1200; // conservative for VPNs etc
    static constexpr size_t UDP_IP_HEADER_SIZE = 28;
    static constexpr size_t DATAGRAM_MAX_SIZE = MTU_MAX - UDP_IP_HEADER_SIZE;
    static constexpr size_t PAYLOAD_MAX_SIZE = DATAGRAM_MAX_SIZE - sizeof(Mold_UDP_64::Downstream_Header);
};
