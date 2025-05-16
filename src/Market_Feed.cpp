#include "Market_Feed.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <bits/this_thread_sleep.h>

#include "Mold_UDP_64.h"

Market_Feed::Market_Feed(
    const std::string& path_to_itch_file,
    const std::string& multicast_group, const uint16_t port, const uint8_t ttl,
    const std::chrono::microseconds throttle_us) : m_sock(socket(AF_INET, SOCK_DGRAM, 0)),
                                                   m_throttle_us(throttle_us),
                                                   m_itch_file(path_to_itch_file)
{
    /* sockaddr config */
    m_multi_addr.sin_family = AF_INET;

    if (port < 1024)
    {
        throw std::invalid_argument(
            std::format("Provide a non privileged port for the multicast feed ( port > 1023) was given {}", port));
    }
    m_multi_addr.sin_port = htons(port);

    if (const int ret = inet_pton(AF_INET, multicast_group.c_str(), &m_multi_addr.sin_addr); ret == 0)
    {
        throw std::invalid_argument(std::format("invalid ip format for multicast group {}",
                                                multicast_group));
    }
    else if (ret < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                std::format("inet_pton failed for multicast group {}", multicast_group));
    }

    if (const uint multi_group_host = ntohl(m_multi_addr.sin_addr.s_addr); multi_group_host < 0xE0000000 ||
        multi_group_host > 0xEFFFFFFF)
    {
        throw std::invalid_argument(std::format("multicast group {} is not a valid IPv4 multicast address "
                                                "(224.0.0.0 - 239.255.255.255).",
                                                multicast_group));
    }

    /* enable reuse_addr, and set ttl + loopback */

    auto opt = 1;
    if (setsockopt(m_sock.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::system_error(errno, std::system_category(), "failure to set SO_REUSEADDR");
    }


    if (setsockopt(m_sock.fd(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        throw std::system_error(errno, std::system_category(), "failed to set multicast ttl");
    }

    opt = 0;
    if (setsockopt(m_sock.fd(), IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt)) < 0)
    {
        throw std::system_error(errno, std::system_category(), "failed to disable multicast loopback");
    }

    /* mmap itch file */

    struct stat sb{};
    if (fstat(m_itch_file.fd(), &sb) < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                std::format("fstat failed for file at path {}", path_to_itch_file));
    }
    m_file_size = sb.st_size;

    m_mapped_file = jam_utils::Memory_Map(m_file_size, PROT_READ, MAP_PRIVATE, m_itch_file.fd(), 0);
}

void Market_Feed::start()
{
    std::array<std::byte, DATAGRAM_MAX_SIZE> datagram{};

    uint64_t mold_seq_num = 1;
    const std::string mold_session = "session001";

    size_t file_pos = 0;
    while (file_pos < m_file_size)
    {
        size_t datagram_pos = 0;

        const auto header = reinterpret_cast<Mold_UDP_64::Downstream_Header*>(datagram.data());
        std::memcpy(header->session, mold_session.c_str(), sizeof(Mold_UDP_64::Downstream_Header::session));
        header->sequence_num = htobe64(mold_seq_num);
        header->msg_count = 0;

        datagram_pos += sizeof(Mold_UDP_64::Downstream_Header);

        while (file_pos < m_file_size)
        {
            if (file_pos + LEN_PREFIX_SIZE > m_file_size)
            {
                std::cerr << std::format(
                    "Unexpected {} trailing bytes at itch file pos {} - not enough for length prefix",
                    m_file_size - file_pos, file_pos);
                break;
            }

            uint16_t msg_len_net;
            std::memcpy(&msg_len_net, &m_mapped_file.get_addr()[file_pos], LEN_PREFIX_SIZE);
            uint16_t msg_len = ntohs(msg_len_net);

            if (msg_len == 0)
            {
                file_pos += LEN_PREFIX_SIZE;
                continue;
            }

            const auto total_msg_size = LEN_PREFIX_SIZE + msg_len;
            if (file_pos + total_msg_size > m_file_size)
            {
                throw std::runtime_error(std::format(
                    "Data error: ITCH message at pos {} has len prefix of {}, which exceeds file size {}", file_pos,
                    msg_len, m_file_size));
            }

            const std::byte* itch_message = &m_mapped_file.get_addr()[file_pos + LEN_PREFIX_SIZE];

            if (datagram_pos + sizeof(Mold_UDP_64::Message_Block_Header) + msg_len >
                DATAGRAM_MAX_SIZE)
            {
                break; // wont fit in current dgram
            }

            const auto block_header = reinterpret_cast<Mold_UDP_64::Message_Block_Header*>(&datagram[
                datagram_pos]);

            block_header->len = htons(msg_len);
            datagram_pos += sizeof(Mold_UDP_64::Message_Block_Header);

            std::memcpy(&datagram[datagram_pos], itch_message, msg_len);
            datagram_pos += msg_len;

            file_pos += total_msg_size;
            header->msg_count++;
            mold_seq_num++;
        }

        if (header->msg_count > 0)
        {
            header->msg_count = htons(header->msg_count);

            ssize_t bytes_sent = sendto(m_sock.fd(), datagram.data(), datagram_pos, 0,
                                        reinterpret_cast<const struct sockaddr*>(&m_multi_addr), sizeof(m_multi_addr));

            if (bytes_sent < 0)
            {
                std::cerr << std::format("sendto failed: {}\n", strerror(errno));
            }
            else if (bytes_sent != datagram_pos)
            {
                std::cerr << std::format("sendto sent {} bytes, but was expected to send {}\n", bytes_sent,
                                         datagram_pos);
            }

            mold_seq_num++;
            std::this_thread::sleep_for(m_throttle_us);
        }

        if (header->msg_count == 0 && file_pos < m_file_size) // exceptional case
        {
            uint16_t msg_len_net;
            std::memcpy(&msg_len_net, &m_mapped_file.get_addr()[file_pos], LEN_PREFIX_SIZE);
            uint16_t msg_len = ntohs(msg_len_net);
            size_t total_msg_size = LEN_PREFIX_SIZE + msg_len;

            std::cerr << std::format("itch message at file position {} "
                                     "with actual length {} "
                                     "(total record size {}) cannot fit into an "
                                     "empty datagram (max payload {}). skipping.",
                                     file_pos, msg_len, total_msg_size, PAYLOAD_MAX_SIZE)
                << std::endl;

            file_pos += total_msg_size;
        }
    }
}
