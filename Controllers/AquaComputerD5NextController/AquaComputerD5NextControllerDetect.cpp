/*---------------------------------------------------------*\
| AquaComputerD5NextControllerDetect.cpp                    |
|                                                           |
|   Inactive D5 NEXT detector; no device access             |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "DetectionManager.h"
#include "LogManager.h"
#include "AquaComputerD5NextController.h"
#include "RGBController_AquaComputerD5Next.h"

#if defined(__linux__) && defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION)
#include <unistd.h>
#endif

/*---------------------------------------------------------*\
| Registration requires an explicit read-only build option |
| or the offline test target. Default builds do no I/O.     |
| No firmware/profile is approved for configuration writes. |
| Scan-time construction sends no feature reports.          |
\*---------------------------------------------------------*/
DetectedControllers DetectAquaComputerD5NextControllers(hid_device_info* info, const std::string& /*name*/)
{
    DetectedControllers detected_controllers;
#if defined(OPENRGB_D5NEXT_OFFLINE_TESTS) || defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION)
    if(info == nullptr || info->vendor_id != D5NEXT_VID || info->product_id != D5NEXT_PID
    || info->interface_number != D5NEXT_INTERFACE || info->path == nullptr || info->path[0] == '\0')
    {
        return detected_controllers;
    }

#if defined(__linux__) && defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION) && !defined(OPENRGB_D5NEXT_OFFLINE_TESTS)
    /*-----------------------------------------------------*\
    | hidapi's hidraw backend opens O_RDWR even when this   |
    | adapter is read-only. Check its permission needs      |
    | before opening; never change permissions here.        |
    \*-----------------------------------------------------*/
    if(access(info->path, R_OK | W_OK) != 0)
    {
        LOG_DEBUG("[AquaComputer D5 NEXT] HID permissions do not allow hidapi opening");
        return detected_controllers;
    }
#endif
    hid_device* dev = hid_open_path(info->path);
    if(dev == nullptr)
    {
        LOG_DEBUG("[AquaComputer D5 NEXT] HID interface is unavailable");
        return detected_controllers;
    }
    AquaComputerD5NextController* controller = new AquaComputerD5NextController(dev, info->path);
    RGBController_AquaComputerD5Next* rgb_controller = new RGBController_AquaComputerD5Next(controller, true);
    detected_controllers.push_back(rgb_controller);
#else
    (void)info;
#endif
    return detected_controllers;
}

#if defined(OPENRGB_D5NEXT_OFFLINE_TESTS) || defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION)
REGISTER_HID_DETECTOR_I("Aqua Computer D5 NEXT", DetectAquaComputerD5NextControllers,
                        D5NEXT_VID, D5NEXT_PID, D5NEXT_INTERFACE);
#endif
