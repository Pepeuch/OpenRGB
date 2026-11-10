/*---------------------------------------------------------*\
| AquaComputerD5NextController.cpp                          |
|                                                           |
|   Staged D5 NEXT lighting controller                       |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "AquaComputerD5NextController.h"
#include <cstring>
#include <stdexcept>

AquaComputerD5NextController::AquaComputerD5NextController(std::unique_ptr<D5NextOwnership> owner,
                                                           D5NextHIDHandle dev_handle)
    : ownership(std::move(owner)), dev(std::move(dev_handle))
{
    if(!ownership || !dev)
    {
        throw std::invalid_argument("D5 NEXT requires an owned path and HID handle");
    }
    location        = std::string("HID: ") + ownership->Path();
    layout          = D5NEXT_LAYOUT_UNKNOWN;
    firmware        = 0;
    write_authorized = false;
    transfer_failed = false;
}

#ifdef OPENRGB_D5NEXT_OFFLINE_TESTS
/*---------------------------------------------------------*\
| The offline target defines HID symbols in-process and     |
| links no HID backend. This override is absent in OpenRGB.  |
\*---------------------------------------------------------*/
AquaComputerD5NextController::AquaComputerD5NextController(std::unique_ptr<D5NextOwnership> owner,
                                                           D5NextHIDHandle dev_handle, D5NextLayout test_layout)
    : AquaComputerD5NextController(std::move(owner), std::move(dev_handle))
{
    layout = test_layout;
    firmware = D5NEXT_FIRMWARE;
    write_authorized = ownership->HasSerial();
}
#endif

AquaComputerD5NextController::~AquaComputerD5NextController()
{
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    dev.reset();
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
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    if(layout != D5NEXT_LAYOUT_DOCUMENTED_809 || !write_authorized)
    {
        return D5NEXT_UNSUPPORTED_LAYOUT;
    }
    unsigned char original[D5NEXT_REPORT_SIZE] = { D5NEXT_REPORT_ID };
    D5NextResult result = ReadFresh(original);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    D5NextIdentity identity = { D5NEXT_VID, D5NEXT_PID, D5NEXT_INTERFACE, firmware };
    D5NextRGBpxConfiguration configuration;
    result = D5NextRGBpx::Decode(original, sizeof(original), identity, configuration);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    const D5NextRGBpxController& group = configuration.groups[D5NEXT_INTERNAL_GROUP];
    if(!off && effect != group.mode)
    {
        return D5NEXT_UNSUPPORTED_TRANSITION;
    }
    D5NextRGBpxPatch patches[3] = {};
    unsigned int count = 0;
    if(!off)
    {
        patches[count].kind  = D5NEXT_PATCH_MASTER;
        patches[count].value = brightness;
        count++;
        if(effect != D5NEXT_EFFECT_RAINBOW)
        {
            D5NextHSV hsv = D5NextRGBpx::RGBToHSV((unsigned char)color, (unsigned char)(color >> 8),
                                                (unsigned char)(color >> 16));
            /* V editing and Breathing's palette role are not established.
             * Never substitute master for a requested per-color value. */
            if(effect != D5NEXT_EFFECT_STATIC || hsv.value != group.colors[0].value)
            {
                return D5NEXT_UNSUPPORTED_OPERATION;
            }
            patches[count].kind       = D5NEXT_PATCH_COLOR;
            patches[count].mask       = D5NEXT_COLOR_H | D5NEXT_COLOR_S;
            patches[count].hue        = hsv.hue;
            patches[count].saturation = hsv.saturation;
            count++;
        }
    }
    patches[count].kind    = D5NEXT_PATCH_STRIP_OFF;
    patches[count].enabled = off;
    count++;
    unsigned char modified[D5NEXT_REPORT_SIZE];
    memcpy(modified, original, sizeof(modified));
    for(unsigned int index = 0; index < count; index++)
    {
        patches[index].group         = D5NEXT_INTERNAL_GROUP;
        patches[index].expected_mode = group.mode;
        unsigned char previous[D5NEXT_REPORT_SIZE];
        memcpy(previous, modified, sizeof(previous));
        result = D5NextRGBpx::BuildPatch(previous, sizeof(previous), identity, patches[index],
                                        modified, sizeof(modified));
        if(result != D5NEXT_OK && result != D5NEXT_UNCHANGED)
        {
            return result;
        }
        result = D5NextRGBpx::AuditPatch(previous, sizeof(previous), identity, patches[index],
                                        modified, sizeof(modified));
        if(result != D5NEXT_OK)
        {
            return result;
        }
    }
    return SendChanged(original, modified);
}

D5NextResult AquaComputerD5NextController::ApplyRGBpxPatch(const D5NextRGBpxPatch& patch)
{
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    if(layout != D5NEXT_LAYOUT_DOCUMENTED_809 || !write_authorized)
    {
        return D5NEXT_UNSUPPORTED_LAYOUT;
    }
    unsigned char original[D5NEXT_REPORT_SIZE] = { D5NEXT_REPORT_ID };
    D5NextResult result = ReadFresh(original);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    D5NextIdentity identity = { D5NEXT_VID, D5NEXT_PID, D5NEXT_INTERFACE, firmware };
    unsigned char candidate[D5NEXT_REPORT_SIZE];
    result = D5NextRGBpx::BuildPatch(original, sizeof(original), identity, patch, candidate, sizeof(candidate));
    if(result != D5NEXT_OK)
    {
        return result;
    }
    result = D5NextRGBpx::AuditPatch(original, sizeof(original), identity, patch, candidate, sizeof(candidate));
    return result == D5NEXT_OK ? SendChanged(original, candidate) : result;
}

/* Called only with ownership's transaction mutex held and authorization checked. */
D5NextResult AquaComputerD5NextController::ReadFresh(unsigned char* report)
{
    if(firmware != D5NEXT_FIRMWARE)
    {
        return D5NEXT_UNSUPPORTED_FIRMWARE;
    }
    if(dev == nullptr || transfer_failed)
    {
        return D5NEXT_NOT_READY;
    }
    int received = hid_get_feature_report(dev.get(), report, D5NEXT_REPORT_SIZE);
    if(received != D5NEXT_REPORT_SIZE)
    {
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }
    return D5NEXT_OK;
}

D5NextResult AquaComputerD5NextController::SendChanged(const unsigned char* original, const unsigned char* candidate)
{
    if(memcmp(original, candidate, D5NEXT_REPORT_SIZE) == 0)
    {
        return D5NEXT_UNCHANGED;
    }
    if(hid_send_feature_report(dev.get(), candidate, D5NEXT_REPORT_SIZE) != D5NEXT_REPORT_SIZE)
    {
        /*-------------------------------------------------*\
        | Completion is uncertain. Stop further transfers;  |
        | do not retry, reset, save, or replay stale data.    |
        \*-------------------------------------------------*/
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }

    /*-----------------------------------------------------*\
    | No save command. Persistence of 0x03 remains unknown. |
    \*-----------------------------------------------------*/
    return D5NEXT_OK;
}
