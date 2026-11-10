/*---------------------------------------------------------*\
| AquaComputerD5NextController.cpp                          |
|                                                           |
|   Staged D5 NEXT lighting controller (not detected)        |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AquaComputerD5NextController.h"

AquaComputerD5NextController::AquaComputerD5NextController(hid_device* dev_handle, const char* path)
{
    dev             = dev_handle;
    location        = path == nullptr ? "" : std::string("HID: ") + path;
    layout          = D5NEXT_LAYOUT_UNKNOWN;
    transfer_failed = false;
}

#ifdef OPENRGB_D5NEXT_OFFLINE_TESTS
/*---------------------------------------------------------*\
| The offline target defines HID symbols in-process and     |
| links no HID backend. This override is absent in OpenRGB.  |
\*---------------------------------------------------------*/
AquaComputerD5NextController::AquaComputerD5NextController(hid_device* dev_handle, const char* path,
                                                         D5NextLayout test_layout)
    : AquaComputerD5NextController(dev_handle, path)
{
    layout = test_layout;
}
#endif

AquaComputerD5NextController::~AquaComputerD5NextController()
{
    std::lock_guard<std::mutex> lock(transaction_mutex);
    if(dev != nullptr)
    {
        hid_close(dev);
    }
}

std::string AquaComputerD5NextController::GetLocation()
{
    return location;
}

D5NextResult AquaComputerD5NextController::SetLighting(unsigned int effect, unsigned int color,
                                                      unsigned int brightness)
{
    if(!D5NextProtocol::IsKnownEffect(effect) || brightness > 255 || (color & 0xFF000000) != 0)
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    return UpdateLighting(false, effect, color, brightness);
}

D5NextResult AquaComputerD5NextController::SetOff()
{
    return UpdateLighting(true, 0, 0, 0);
}

D5NextResult AquaComputerD5NextController::UpdateLighting(bool off, unsigned int effect,
                                                         unsigned int color, unsigned int brightness)
{
    /*-----------------------------------------------------*\
    | Lock the entire fresh-read / validate / modify /      |
    | compare / write transaction, including error paths.   |
    | This does not protect against other processes.        |
    \*-----------------------------------------------------*/
    std::lock_guard<std::mutex> lock(transaction_mutex);
    if(layout != D5NEXT_LAYOUT_DOCUMENTED_809)
    {
        return D5NEXT_UNSUPPORTED_LAYOUT;
    }
    if(dev == nullptr || transfer_failed)
    {
        return D5NEXT_NOT_READY;
    }

    unsigned char original[D5NEXT_REPORT_SIZE] = { D5NEXT_REPORT_ID };
    int received = hid_get_feature_report(dev, original, sizeof(original));
    if(received != D5NEXT_REPORT_SIZE)
    {
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }
    D5NextLighting lighting;
    D5NextResult result = D5NextProtocol::Decode(original, sizeof(original), layout, lighting);
    if(result != D5NEXT_OK)
    {
        return result;
    }

    if(off)
    {
        lighting.enabled = D5NEXT_LIGHTING_OFF;
    }
    else
    {
        lighting.effect  = effect;
        lighting.enabled = D5NEXT_LIGHTING_ON;
        if(effect != D5NEXT_EFFECT_RAINBOW)
        {
            /*---------------------------------------------*\
            | OpenRGB RGBColor is 0x00BBGGRR. Multiply HSV  |
            | value by the separate mode brightness.        |
            \*---------------------------------------------*/
            D5NextLighting hsv = D5NextProtocol::RGBToHSV((unsigned char)color,
                                                       (unsigned char)(color >> 8),
                                                       (unsigned char)(color >> 16));
            lighting.hue        = hsv.hue;
            lighting.saturation = hsv.saturation;
            lighting.brightness = (hsv.brightness * brightness + 127) / 255;
        }
        else
        {
            lighting.brightness = brightness;
        }
    }

    unsigned char modified[D5NEXT_REPORT_SIZE];
    result = D5NextProtocol::BuildLightingReport(original, sizeof(original), layout, lighting,
                                                modified, sizeof(modified));
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(hid_send_feature_report(dev, modified, sizeof(modified)) != D5NEXT_REPORT_SIZE)
    {
        /*-------------------------------------------------*\
        | Completion is uncertain. Stop further transfers;  |
        | do not retry, reset, save, or replay stale data.    |
        \*-------------------------------------------------*/
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }

    /*-----------------------------------------------------*\
    | No report 0x02: persistence is deliberately absent.   |
    \*-----------------------------------------------------*/
    return D5NEXT_OK;
}
