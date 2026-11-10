/*---------------------------------------------------------*\
| AquaComputerD5NextController.h                            |
|                                                           |
|   Staged D5 NEXT lighting controller (not detected)        |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <mutex>
#include <string>
#include <hidapi.h>
#include "D5NextProtocol.h"

class AquaComputerD5NextController
{
public:
    /*-----------------------------------------------------*\
    | Takes ownership of an already-open handle. Constructor |
    | sends nothing. Production has no approved firmware    |
    | profile and refuses all feature-report access.        |
    \*-----------------------------------------------------*/
    AquaComputerD5NextController(hid_device* dev_handle, const char* path);
#ifdef OPENRGB_D5NEXT_OFFLINE_TESTS
    AquaComputerD5NextController(hid_device* dev_handle, const char* path, D5NextLayout test_layout);
#endif
    ~AquaComputerD5NextController();

    std::string GetLocation();
    D5NextResult SetLighting(unsigned int effect, unsigned int color, unsigned int brightness);
    D5NextResult SetOff();

private:
    hid_device*     dev;
    std::string     location;
    D5NextLayout    layout;
    bool            transfer_failed;
    std::mutex      transaction_mutex;

    D5NextResult UpdateLighting(bool off, unsigned int effect, unsigned int color, unsigned int brightness);
};
