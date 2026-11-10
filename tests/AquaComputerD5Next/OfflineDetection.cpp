/*---------------------------------------------------------*\
| OfflineDetection.cpp                                     |
|                                                           |
|   Test registry; no discovery thread or hardware backend  |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "OfflineDetection.h"

static std::vector<HIDDeviceDetectorBlock>& OfflineHIDDetectors()
{
    static std::vector<HIDDeviceDetectorBlock> detectors;
    return detectors;
}

const std::vector<HIDDeviceDetectorBlock>& GetOfflineHIDDetectors()
{
    return OfflineHIDDetectors();
}

/*---------------------------------------------------------*\
| Replace only manager startup/registration at link time.   |
| Use the real header, block type, and registration macro.  |
\*---------------------------------------------------------*/
DetectionManager::DetectionManager()
{
}

DetectionManager::~DetectionManager()
{
}

DetectionManager* DetectionManager::get()
{
    static DetectionManager manager;
    return &manager;
}

void DetectionManager::RegisterHIDDeviceDetector(std::string name, HIDDeviceDetectorFunction detector,
                                                int vid, int pid, int interface, int usage_page,
                                                int usage, bool enabled_by_default, int bus)
{
    HIDDeviceDetectorBlock block;
    block.name               = name;
    block.function           = detector;
    block.vid                = vid;
    block.pid                = pid;
    block.interface          = interface;
    block.usage_page         = usage_page;
    block.usage              = usage;
    block.enabled_by_default = enabled_by_default;
    block.bus                = bus;
    OfflineHIDDetectors().push_back(block);
}
