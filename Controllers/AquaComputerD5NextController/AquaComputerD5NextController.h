/*---------------------------------------------------------*\
| AquaComputerD5NextController.h                            |
|                                                           |
|   Staged D5 NEXT lighting controller                       |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <string>
#include "D5NextOwnership.h"
#include "D5NextRGBpx.h"

class AquaComputerD5NextController
{
public:
    /*-----------------------------------------------------*\
    | Takes ownership of a reserved path and open handle.    |
    | Constructor sends nothing. ReadConfiguration is an    |
    | explicit read-only path, never write authorization.   |
    \*-----------------------------------------------------*/
    AquaComputerD5NextController(std::unique_ptr<D5NextOwnership> ownership, D5NextHIDHandle dev_handle);
#ifdef OPENRGB_D5NEXT_OFFLINE_TESTS
    AquaComputerD5NextController(std::unique_ptr<D5NextOwnership> ownership, D5NextHIDHandle dev_handle,
                                 D5NextLayout test_layout);
#endif
    ~AquaComputerD5NextController();

    std::string GetLocation();
    D5NextResult ReadConfiguration();
    bool GetConfiguration(D5NextRGBpxConfiguration& snapshot);
    unsigned int GetFirmware();
    D5NextResult SetLighting(unsigned int effect, unsigned int color, unsigned int brightness);
    D5NextResult SetOff();
    D5NextResult ApplyRGBpxPatch(const D5NextRGBpxPatch& patch);

private:
    std::unique_ptr<D5NextOwnership> ownership;
    D5NextHIDHandle dev;
    std::string     location;
    D5NextLayout    layout;
    unsigned int    firmware;
    bool            write_authorized;
    bool            transfer_failed;
    bool            read_attempted = false;
    bool            configuration_valid = false;
    D5NextRGBpxConfiguration configuration = {};

    D5NextResult UpdateLighting(bool off, unsigned int effect, unsigned int color, unsigned int brightness);
    D5NextResult ReadFresh(unsigned char* report);
    D5NextResult SendChanged(const unsigned char* original, const unsigned char* candidate);
};
