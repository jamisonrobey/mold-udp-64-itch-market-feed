#pragma once

#include <cstdint>

namespace Mold_UDP_64
{
#pragma pack(push, 1)

    struct Downstream_Header {
        char session[10];
        uint64_t sequence_num;
        uint16_t msg_count;
    };

    struct Message_Block_Header {
        uint16_t len;
    };

#pragma pack(pop)
};
