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

/* Geometry only: decoded allocations do not prove physical connectivity.
 * The known g6/g7 overlap is retained as metadata, never a third output. */
static bool ValidAllocations(const D5NextRGBpxConfiguration& snapshot)
{
    const auto& pump = snapshot.groups[6];
    if(pump.strip != 1 || pump.led_start != 0 || pump.count != 6) return false;
    for(unsigned int i = 0; i < 8; i++)
    {
        const auto& group = snapshot.groups[i];
        unsigned int limit = i < 6 ? 90 : 6;
        if(group.strip != (i < 6 ? 0 : 1) || group.led_start > limit
        || group.count > limit - group.led_start || (group.mode != 0 && group.count == 0)
        || D5NextEffects::Find(group.mode) == nullptr) return false;
        if(i < 6 && group.mode != 0 && group.count != 0)
        {
            for(unsigned int j = 0; j < i; j++)
            {
                const auto& other = snapshot.groups[j];
                if(other.mode != 0 && other.count != 0
                && group.led_start < other.led_start + other.count
                && other.led_start < group.led_start + group.count) return false;
            }
        }
    }
    const auto& overlay = snapshot.groups[7];
    return overlay.mode == 0 || (overlay.led_start == 0 && overlay.count == 6);
}

D5NextResult AquaComputerD5NextController::ReadConfiguration()
{
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    write_authorized = false;
    if(read_attempted || transfer_failed) return D5NEXT_NOT_READY;
    read_attempted = true;
    wchar_t serial[256] = {};
    if(!ownership->HasSerial() || hid_get_serial_number_string(dev.get(), serial, 256) != 0
    || serial[255] != 0 || !ownership->MatchesSerial(serial)) return D5NEXT_PRECONDITION_FAILED;
    unsigned char status[163] = {};
    /* Passive periodic Input01, bounded wait, no retry or status control GET. */
    int received = hid_read_timeout(dev.get(), status, sizeof(status), 2000);
    if(received != 162 || status[0] != 1)
    {
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }
    unsigned int observed_firmware = (status[13] << 8) | status[14];
    if(observed_firmware != D5NEXT_FIRMWARE) return D5NEXT_UNSUPPORTED_FIRMWARE;
    unsigned char report[809] = { 3 };
    if(hid_get_feature_report(dev.get(), report, sizeof(report)) != 809)
    {
        transfer_failed = true;
        return D5NEXT_IO_ERROR;
    }
    D5NextRGBpxConfiguration decoded;
    const D5NextIdentity identity = { D5NEXT_VID, D5NEXT_PID, D5NEXT_INTERFACE, observed_firmware };
    auto result = D5NextRGBpx::Decode(report, sizeof(report), identity, decoded);
    if(result != D5NEXT_OK) return result;
    if(!ValidAllocations(decoded)) return D5NEXT_PRECONDITION_FAILED;
    configuration = decoded;
    firmware = observed_firmware;
    layout = D5NEXT_LAYOUT_DOCUMENTED_809;
    configuration_valid = true;
    return D5NEXT_OK;
}

bool AquaComputerD5NextController::GetConfiguration(D5NextRGBpxConfiguration& snapshot)
{
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    if(!configuration_valid) return false;
    snapshot = configuration;
    return true;
}

unsigned int AquaComputerD5NextController::GetFirmware()
{
    std::lock_guard<std::mutex> lock(ownership->transaction_mutex);
    return firmware;
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
