# ITCH 5.0 TotalView feed over MoldUDP64

- ITCH 5.0 TotalView feed by replaying historical ITCH data over MoldUDP64.
- **Use-case**: provides a local testing environment for developing applications that consume exchange-like ITCH 5.0 data feeds over MoldUDP64

# Functionality

- Reads raw ITCH 5.0 messages from a binary file.
- Packs these ITCH messages into MoldUDP64 datagrams.
- Multicast feed and retransmission feeds

# Using

## Build

- Unix only
- add steps for ubunutu / wsl

## Input File Format

- This tool works **only** with historical ITCH binary files from [emi.nasdaq.com](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/).
- I'm unaware of the format of more recent dumps as they are paid so I'm just working with this as it's free.
- The input ITCH file is a sequence of records. Each record is:
    - Length Prefix (2 bytes): Unsigned 16-bit integer, BIG-ENDIAN. Specifies the length (L) of the ITCH payload that follows.
    - ITCH Payload (L bytes): The actual ITCH 5.0 message.
    - `[uint16_t BE_LEN L][PAYLOAD[L]]`
 
