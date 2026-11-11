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
| Configuration snapshot only; no writable GUI capability.  |
| The optional read-only detector is disabled by default.   |
\*---------------------------------------------------------*/
RGBController_AquaComputerD5Next::RGBController_AquaComputerD5Next(
    std::unique_ptr<AquaComputerD5NextController> controller_ptr)
    : controller(std::move(controller_ptr))
{
    name        = "Aqua Computer D5 NEXT";
    vendor      = "Aqua Computer";
    description = "Experimental RGBpx configuration snapshot; read only";
    type        = DEVICE_TYPE_COOLER;
    location    = controller->GetLocation();
    version     = controller->GetFirmware() == D5NEXT_FIRMWARE ? "1027" : "Unknown";

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
    description = "Experimental RGBpx configuration snapshot; read only";
    zones.clear();
    leds.clear();
    D5NextRGBpxConfiguration snapshot;
    if(!controller->GetConfiguration(snapshot)) return;
    const unsigned int groups[] = { 6, 0 };
    const char* names[] = { "Pump RGB", "External RGBpx" };
    for(unsigned int index = 0; index < 2; index++)
    {
        const auto& group = snapshot.groups[groups[index]];
        if(index == 1 && (group.count == 0 || group.mode == 0)) continue;
        zone observed;
        observed.name       = names[index];
        observed.type       = ZONE_TYPE_LINEAR;
        observed.leds_min   = group.count;
        observed.leds_max   = group.count;
        observed.leds_count = group.count;
        zones.push_back(observed);
        for(unsigned int position = 0; position < group.count; position++)
        {
            led item;
            item.name = std::string(names[index]) + " LED " + std::to_string(position + 1);
            leds.push_back(item);
        }
    }
    if(snapshot.groups[7].mode != 0)
        description += "; g7 overlaps Pump RGB, rendering priority unknown";
    SetupColors();
}

void RGBController_AquaComputerD5Next::DeviceUpdateLEDs()
{
    DeviceUpdateMode();
}

void RGBController_AquaComputerD5Next::DeviceUpdateZoneLEDs(int zone)
{
    if(zone >= 0 && (unsigned int)zone < zones.size())
    {
        DeviceUpdateMode();
    }
}

void RGBController_AquaComputerD5Next::DeviceUpdateSingleLED(int led)
{
    if(led >= 0 && (unsigned int)led < leds.size())
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
