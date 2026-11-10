/*---------------------------------------------------------*\
| RGBController_AquaComputerD5Next.h                         |
|                                                           |
|   Staged D5 NEXT global lighting adapter                  |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
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
    explicit RGBController_AquaComputerD5Next(std::unique_ptr<AquaComputerD5NextController> controller_ptr);
    ~RGBController_AquaComputerD5Next();

    void SetupZones();
    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;

private:
    std::unique_ptr<AquaComputerD5NextController> controller;
};
