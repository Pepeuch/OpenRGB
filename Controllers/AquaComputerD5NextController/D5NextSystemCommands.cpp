/*---------------------------------------------------------*\
| D5NextSystemCommands.cpp                                  |
|                                                           |
|   Offline SettingsSave encoder from documented captures   |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstring>
#include "D5NextSystemCommands.h"

D5NextResult D5NextSystemCommands::EncodeSettingsSave(unsigned char* output, std::size_t size)
{
    if(output == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(size != D5NEXT_SYSTEM_REPORT_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    unsigned char command[D5NEXT_SYSTEM_REPORT_SIZE] = { 0x02, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0 };
    unsigned short crc = D5NextProtocol::CalculateCRC(command + 1, 8);
    command[9]  = (unsigned char)(crc >> 8);
    command[10] = (unsigned char)crc;
    memcpy(output, command, sizeof(command));
    return D5NEXT_OK;
}
