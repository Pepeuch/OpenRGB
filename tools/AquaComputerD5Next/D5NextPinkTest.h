/*---------------------------------------------------------*\
| D5NextPinkTest.h                                          |
|                                                           |
|   Audited one-shot D5 NEXT pink test preparation           |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <mutex>
#include "D5NextReadOnly.h"

struct D5NextPinkAudit
{
    D5NextLighting initial;
    D5NextLighting target;
    unsigned short initial_crc;
    unsigned short calculated_initial_crc;
    unsigned short target_crc;
    unsigned int changed_count;
    unsigned int offsets[8];
    unsigned char before[8];
    unsigned char after[8];
};

class D5NextPinkTest
{
public:
    D5NextPinkTest();
    ~D5NextPinkTest();

    static D5NextResult BuildPlan(const unsigned char* original, std::size_t size,
                                  unsigned char* candidate, std::size_t candidate_size,
                                  D5NextPinkAudit& audit);
    static D5NextResult AuditPlan(const unsigned char* original, std::size_t size,
                                  const unsigned char* candidate, std::size_t candidate_size,
                                  D5NextPinkAudit& audit);
    static bool FirmwareIsApproved(unsigned int firmware);
    static void PrintAudit(const D5NextPinkAudit& audit);

    /*-----------------------------------------------------*\
    | One invocation per instance. Holds one handle and its |
    | cooperative lock through status/GET/audit/optional SET.|
    | No production firmware is approved, so writes refuse. |
    \*-----------------------------------------------------*/
    int Run(const D5NextDeviceInfo& device, const D5NextDescriptor& descriptor,
             bool competing_process, bool write_once, bool recovery_confirmed,
             bool process_inventory_complete = false);

private:
    int fd;
    bool consumed;
    std::mutex transaction_mutex;
    int Finish(int result);
};
