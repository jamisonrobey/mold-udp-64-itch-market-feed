#include "Market_Feed.h"
#include <iostream>
#include <format>
#include <sstream>
#include <string>

int main(const int argc, char* argv[])
{
    if (argc != 6)
    {
        std::cerr << std::format(
            "Usage: {} <multicast-group> <port> <itch-file-path> <ttl> <throttle-us>\n",
            argv[0]
        );
        return 1;
    }

    std::string feed_multicast_group = argv[1];

    int feed_port;
    if (const std::stringstream ss(argv[2]); !ss >> feed_port || feed_port <= 0)
    {
        std::cerr << std::format("Invalid port number: {}\n", argv[2]);
        return 1;
    }

    std::string itch_file_path = argv[3];

    int feed_ttl;
    if (std::stringstream ss(argv[4]); !(ss >> feed_ttl) || feed_ttl < 0)
    {
        std::cerr << std::format("Invalid feed ttl: {}\n", argv[4]);
        return 1;
    }

    int feed_throttle_us;
    if (std::stringstream ss(argv[5]); !(ss >> feed_throttle_us) || feed_throttle_us < 0)
    {
        std::cerr << std::format("Invalid feed throttle us: {}\n", argv[5]);
        return 1;
    }

    try
    {
        Market_Feed feed(itch_file_path, feed_multicast_group, feed_port, feed_ttl,
                         std::chrono::microseconds(feed_throttle_us));
        feed.start();
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
