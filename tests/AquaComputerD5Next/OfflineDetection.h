/*---------------------------------------------------------*\
| OfflineDetection.h                                       |
|                                                           |
|   Test registry for the actual HID registration macro     |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "DetectionManager.h"

const std::vector<HIDDeviceDetectorBlock>& GetOfflineHIDDetectors();
