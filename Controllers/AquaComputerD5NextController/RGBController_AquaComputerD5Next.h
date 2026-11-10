/*---------------------------------------------------------*\
| RGBController_AquaComputerD5Next.h                         |
|                                                           |
|   Staged D5 NEXT global lighting adapter                  |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "AquaComputerD5NextController.h"
#include "RGBController.h"

class RGBController_AquaComputerD5Next : public RGBController
{
public:
    RGBController_AquaComputerD5Next(AquaComputerD5NextController* controller_ptr, bool read_only = false);
    ~RGBController_AquaComputerD5Next();

    void SetupZones();
    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

private:
    AquaComputerD5NextController* controller;
    bool read_only;
    void LogResult(D5NextResult result);
};
