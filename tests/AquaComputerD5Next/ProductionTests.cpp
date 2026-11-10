/*---------------------------------------------------------*\
| ProductionTests.cpp                                       |
|                                                           |
|   Real production guards, in-process HID sentinels only    |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstdio>
#include <cstdlib>
#include "DetectionManager.h"
#include "RGBController_AquaComputerD5Next.h"

#if defined(OPENRGB_D5NEXT_OFFLINE_TESTS) || defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION)
#error Production guard tests must compile without either detection/test macro
#endif

static unsigned int checks;
static unsigned int closes;
#define CHECK(condition) Check((condition), #condition, __LINE__)
static void Check(bool condition, const char* expression, int line)
{
    checks++;
    if(!condition)
    {
        std::fprintf(stderr, "FAIL production line %d: %s\n", line, expression);
        std::exit(1);
    }
}

struct hid_device_ { int unused; };
hid_device* hid_open_path(const char*)
{
    CHECK(false);
    return nullptr;
}
int hid_get_feature_report(hid_device*, unsigned char*, std::size_t)
{
    CHECK(false);
    return -1;
}
int hid_send_feature_report(hid_device*, const unsigned char*, std::size_t)
{
    CHECK(false);
    return -1;
}
void hid_close(hid_device*)
{
    closes++;
}

DetectedControllers DetectAquaComputerD5NextControllers(hid_device_info*, const std::string&);

int main()
{
    hid_device_ sentinel;
    hid_device_info info = {};
    char path[] = "/offline/production";
    info.path = path;
    info.vendor_id = 0x0C70;
    info.product_id = 0xF00E;
    info.interface_number = 1;
    info.release_number = 1027; /* Must never be treated as permission. */
    CHECK(DetectAquaComputerD5NextControllers(&info, "test").empty());
    CHECK(DetectAquaComputerD5NextControllers(nullptr, "test").empty());
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire(path, L"production-sentinel"),
                                                D5NextHIDHandle(&sentinel));
        CHECK(controller.SetLighting(1, 0xB469FF, 255) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(controller.SetOff() == D5NEXT_UNSUPPORTED_LAYOUT);
        for(unsigned int mode = 0; mode < 25; mode++)
        {
            D5NextRGBpxPatch patch = {};
            patch.group = 6;
            patch.expected_mode = mode;
            for(unsigned int kind = D5NEXT_PATCH_MASTER; kind <= D5NEXT_PATCH_MODE; kind++)
            {
                patch.kind = (D5NextPatchKind)kind;
                CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_UNSUPPORTED_LAYOUT);
            }
        }
    }
    CHECK(closes == 1);
    for(unsigned int iteration = 0; iteration < 2; iteration++)
    {
        RGBController_AquaComputerD5Next rgb(std::unique_ptr<AquaComputerD5NextController>(
            new AquaComputerD5NextController(D5NextOwnership::Acquire(path, L"production-sentinel"),
                                             D5NextHIDHandle(&sentinel))));
        CHECK(rgb.GetLEDCount() == 6 && rgb.GetZoneCount() == 1 && !rgb.SupportsPerZoneModes());
        CHECK(rgb.GetModeCount() == 1 && rgb.GetModeName(0) == "Read only");
        rgb.DeviceUpdateMode();
        rgb.DeviceUpdateLEDs();
        rgb.DeviceUpdateZoneLEDs(0);
        rgb.DeviceUpdateSingleLED(5);
        rgb.SetDeviceSpecificConfiguration({ { "firmware", 1027 }, { "write_authorized", true } });
        rgb.SetDeviceSpecificZoneConfiguration(0, { { "group", 0 }, { "value0", 44 } });
        rgb.SetAllColors(0xB469FF);
        rgb.SetActiveMode(1);
        CHECK(rgb.GetActiveMode() == 0);
        rgb.UpdateSingleLED(0);
        rgb.SaveMode();
    }
    CHECK(closes == 3);
    std::printf("PASS production detector/layout/authorization/profile guards (%u checks); HID calls forbidden\n", checks);
    return 0;
}
