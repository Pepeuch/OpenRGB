/*---------------------------------------------------------*\
| D5NextSystemCommands.h                                    |
|                                                           |
|   Offline encoding only; no command transport             |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once
#include "D5NextProtocol.h"

#define D5NEXT_SYSTEM_REPORT_ID      0x02
#define D5NEXT_SYSTEM_REPORT_SIZE    11
#define D5NEXT_SAVE_OUTPUT_WVALUE    0x0202

class D5NextSystemCommands
{
public:
    /* Observed SET_REPORT Output on EP0/IF1, not Feature.
     * No send method or automatic save is provided. */
    static D5NextResult EncodeSettingsSave(unsigned char* output, std::size_t size);
};
