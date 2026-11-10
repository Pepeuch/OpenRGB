/*---------------------------------------------------------*\
| RGBController_AquaComputerD5Next.cpp                       |
|                                                           |
|   Staged D5 NEXT global lighting adapter                  |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AquaComputerD5Next.h"

/*---------------------------------------------------------*\
| No supported-device metadata until hardware validation.   |
| The optional read-only detector is disabled by default.   |
\*---------------------------------------------------------*/
RGBController_AquaComputerD5Next::RGBController_AquaComputerD5Next(
    std::unique_ptr<AquaComputerD5NextController> controller_ptr)
    : controller(std::move(controller_ptr))
{
    name        = "Aqua Computer D5 NEXT";
    vendor      = "Aqua Computer";
    description = "D5 NEXT read-only discovery; configuration writes disabled";
    type        = DEVICE_TYPE_COOLER;
    location    = controller->GetLocation();

    mode observed;
    observed.name       = "Read only";
    observed.color_mode = MODE_COLORS_NONE;
    modes.push_back(observed);
    SetupZones();
}

RGBController_AquaComputerD5Next::~RGBController_AquaComputerD5Next()
{
    Shutdown();
}

void RGBController_AquaComputerD5Next::SetupZones()
{
    zone global_zone;
    global_zone.name       = "Pump LEDs";
    global_zone.type       = ZONE_TYPE_LINEAR;
    global_zone.leds_min   = 6;
    global_zone.leds_max   = 6;
    global_zone.leds_count = 6;
    zones.push_back(global_zone);

    for(unsigned int index = 0; index < 6; index++)
    {
        led pump_led;
        pump_led.name = "Pump LED " + std::to_string(index + 1);
        leds.push_back(pump_led);
    }
    SetupColors();
}

void RGBController_AquaComputerD5Next::DeviceUpdateLEDs()
{
    DeviceUpdateMode();
}

void RGBController_AquaComputerD5Next::DeviceUpdateZoneLEDs(int zone)
{
    if(zone == 0)
    {
        DeviceUpdateMode();
    }
}

void RGBController_AquaComputerD5Next::DeviceUpdateSingleLED(int led)
{
    if(led >= 0 && led < 6)
    {
        DeviceUpdateMode();
    }
}

void RGBController_AquaComputerD5Next::DeviceUpdateMode()
{
    /*-----------------------------------------------------*\
    | Profile loading and every color/mode callback are     |
    | inert for read-only discovery, before controller I/O. |
    \*-----------------------------------------------------*/
    return;
}
