/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <cstddef>
#include <cstring>
#include "D5NextProtocol.h"

/* Test-only descriptions of the observed forms, not a general USB protocol.
 * No encoder, transport or interpretation of the two header 06 bytes. */
struct DocumentaryColor { unsigned char green, red, blue; };

inline bool DecodeObservedBulk35(const unsigned char* bytes, std::size_t size)
{
    if(bytes == nullptr || size != 35 || bytes[0] != 0x0A) return false;
    for(unsigned int index = 1; index <= 32; index++)
        if(bytes[index] != 0) return false; /* Nonzero audio is unknown. */
    return D5NextProtocol::CalculateCRC(bytes + 1, 32)
        == (unsigned short)((bytes[33] << 8) | bytes[34]);
}

inline bool DecodeObservedBulk364(const unsigned char* bytes, std::size_t size,
                                 DocumentaryColor (&output)[6])
{
    if(bytes == nullptr || size != 364) return false;
    const unsigned char header[] = { 0x0B, 6, 6, 0 };
    if(std::memcmp(bytes, header, sizeof(header)) != 0) return false;
    for(unsigned int index = 28; index < 364; index++)
        if(bytes[index] != 0) return false;
    DocumentaryColor colors[6];
    for(unsigned int index = 0; index < 6; index++)
    {
        unsigned int offset = 4 + 4 * index;
        if(bytes[offset] != 0) return false;
        colors[index] = { bytes[offset + 1], bytes[offset + 2], bytes[offset + 3] };
    }
    std::memcpy(output, colors, sizeof(colors));
    return true;
}
