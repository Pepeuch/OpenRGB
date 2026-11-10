/*---------------------------------------------------------*\
| RGBController_AquaComputerD5Next.cpp                       |
|                                                           |
|   Staged D5 NEXT global lighting adapter                  |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "LogManager.h"
#include "RGBController_AquaComputerD5Next.h"

/*---------------------------------------------------------*\
| No supported-device metadata until hardware validation.   |
| The optional read-only detector is disabled by default.   |
\*---------------------------------------------------------*/
RGBController_AquaComputerD5Next::RGBController_AquaComputerD5Next(AquaComputerD5NextController* controller_ptr,
                                                                 bool read_only_value)
{
    controller  = controller_ptr;
    read_only   = read_only_value;
    name        = "Aqua Computer D5 NEXT";
    vendor      = "Aqua Computer";
    description = "D5 NEXT global lighting (unvalidated prototype)";
    type        = DEVICE_TYPE_COOLER;
    location    = controller->GetLocation();

    if(read_only)
    {
        description = "D5 NEXT read-only discovery; configuration writes disabled";
        mode observed;
        observed.name       = "Read only";
        observed.color_mode = MODE_COLORS_NONE;
        modes.push_back(observed);
        SetupZones();
        return;
    }

    const char* mode_names[] = { "Off", "Static", "Breathing", "Rainbow" };
    const unsigned int mode_values[] = { 0, D5NEXT_EFFECT_STATIC, D5NEXT_EFFECT_BREATHING, D5NEXT_EFFECT_RAINBOW };
    for(unsigned int index = 0; index < 4; index++)
    {
        mode new_mode;
        new_mode.name       = mode_names[index];
        new_mode.value      = mode_values[index];
        new_mode.flags      = MODE_FLAG_REQUIRES_ENTIRE_DEVICE;
        new_mode.color_mode = MODE_COLORS_NONE;
        if(index != 0)
        {
            new_mode.flags          |= MODE_FLAG_HAS_BRIGHTNESS;
            new_mode.brightness_min = 0;
            new_mode.brightness_max = 255;
            new_mode.brightness     = 255;
        }
        if(index == 1 || index == 2)
        {
            new_mode.flags         |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
            new_mode.color_mode     = MODE_COLORS_MODE_SPECIFIC;
            new_mode.colors_min     = 1;
            new_mode.colors_max     = 1;
            new_mode.colors.push_back(0x00FFFFFF);
        }
        modes.push_back(new_mode);
    }

    SetupZones();
}

RGBController_AquaComputerD5Next::~RGBController_AquaComputerD5Next()
{
    Shutdown();
    delete controller;
}

void RGBController_AquaComputerD5Next::SetupZones()
{
    zone global_zone;
    global_zone.name       = "Global lighting";
    global_zone.type       = ZONE_TYPE_SINGLE;
    global_zone.leds_min   = 1;
    global_zone.leds_max   = 1;
    global_zone.leds_count = 1;
    zones.push_back(global_zone);

    led global_led;
    global_led.name = "Global lighting";
    leds.push_back(global_led);
    SetupColors();
}

void RGBController_AquaComputerD5Next::LogResult(D5NextResult result)
{
    if(result != D5NEXT_OK && result != D5NEXT_UNCHANGED)
    {
        LOG_WARNING("[AquaComputer D5 NEXT] Lighting update refused: %s", D5NextProtocol::ResultString(result));
    }
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
    if(led == 0)
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
    if(read_only)
    {
        return;
    }
    if(active_mode < 0 || (unsigned int)active_mode >= modes.size())
    {
        LogResult(D5NEXT_INVALID_LIGHTING);
        return;
    }
    const mode& current = modes[active_mode];
    if(current.value == 0)
    {
        LogResult(controller->SetOff());
        return;
    }

    unsigned int color = 0;
    if(current.flags & MODE_FLAG_HAS_MODE_SPECIFIC_COLOR)
    {
        if(current.color_mode != MODE_COLORS_MODE_SPECIFIC || current.colors.size() != 1)
        {
            LogResult(D5NEXT_INVALID_LIGHTING);
            return;
        }
        color = current.colors[0];
    }
    LogResult(controller->SetLighting(current.value, color, current.brightness));
}
