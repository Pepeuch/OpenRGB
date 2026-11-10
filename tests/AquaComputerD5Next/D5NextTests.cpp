/*---------------------------------------------------------*\
| D5NextTests.cpp                                           |
|                                                           |
|   Synthetic reports and in-process HID mocks only         |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "OfflineDetection.h"
#include "RGBController_AquaComputerD5Next.h"

#define CHECK(condition) Check((condition), #condition, __LINE__)

static unsigned int checks = 0;

static void Check(bool condition, const char* expression, int line)
{
    checks++;
    if(!condition)
    {
        std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
        std::exit(EXIT_FAILURE);
    }
}

/*---------------------------------------------------------*\
| Independent, non-reflected CRC oracle for fixtures.       |
| Written from CRC parameters, not from aquapx sources.      |
\*---------------------------------------------------------*/
static unsigned short ReferenceCRC(const unsigned char* bytes, std::size_t size)
{
    unsigned int remainder = 0xFFFF;
    for(std::size_t index = 0; index < size; index++)
    {
        for(unsigned int bit = 0; bit < 8; bit++)
        {
            unsigned int input = (bytes[index] >> bit) & 1;
            unsigned int carry = ((remainder >> 15) & 1) ^ input;
            remainder = (remainder << 1) & 0xFFFF;
            if(carry != 0)
            {
                remainder ^= 0x8005;
            }
        }
    }
    unsigned int reflected = 0;
    for(unsigned int bit = 0; bit < 16; bit++)
    {
        reflected = (reflected << 1) | ((remainder >> bit) & 1);
    }
    return (unsigned short)(reflected ^ 0xFFFF);
}

static void FixCRC(unsigned char* report)
{
    unsigned short crc = ReferenceCRC(report + 1, 806);
    report[807] = (unsigned char)(crc >> 8);
    report[808] = (unsigned char)crc;
}

/*---------------------------------------------------------*\
| Artificial patterned buffer, NOT a captured device report.|
| Offsets in tests deliberately do not use codec constants. |
\*---------------------------------------------------------*/
static void MakeFixture(unsigned char* report, unsigned int seed = 19)
{
    for(unsigned int index = 0; index < 809; index++)
    {
        report[index] = (unsigned char)(index * 73 + seed);
    }
    report[0]   = 0x03;
    report[1]   = 0;
    report[2]   = 3;
    report[237] = 87;
    report[239] = 0x00;
    report[660] = 1;
    report[661] = 0;
    report[662] = 6;
    report[663] = 0x01;
    report[706] = 0x01;
    report[707] = 0xFF;
    report[708] = 155;
    report[709] = 255;
    FixCRC(report);
}

static bool AllowedOffset(unsigned int offset)
{
    return offset == 237 || offset == 239 || offset == 663 || offset == 706
        || offset == 707 || offset == 708 || offset == 807 || offset == 808;
}

static void CheckPreservation(const unsigned char* original, const unsigned char* modified)
{
    for(unsigned int offset = 0; offset < 809; offset++)
    {
        if(!AllowedOffset(offset))
        {
            CHECK(original[offset] == modified[offset]);
        }
    }
    CHECK(ReferenceCRC(modified + 1, 806) == (unsigned short)((modified[807] << 8) | modified[808]));
}

static void TestCRCAndFraming()
{
    const unsigned char check[] = "123456789";
    CHECK(ReferenceCRC(check, 9) == 0xB4C8);
    CHECK(D5NextProtocol::CalculateCRC(check, 9) == 0xB4C8);
    CHECK(D5NextProtocol::CalculateCRC(check, 0) == 0x0000);
    /* Report 0x02 is examined offline, never transmitted. */
    const unsigned char save_body[] = { 0, 0, 0, 2, 0, 0, 0, 0 };
    CHECK(ReferenceCRC(save_body, sizeof(save_body)) == 0x34C6);
    CHECK(D5NextProtocol::CalculateCRC(save_body, sizeof(save_body)) == 0x34C6);

    unsigned char report[810];
    MakeFixture(report);
    CHECK(D5NextProtocol::ValidateReport(report, 809) == D5NEXT_OK);
    for(unsigned int length = 0; length < 809; length++)
    {
        CHECK(D5NextProtocol::ValidateReport(report, length) == D5NEXT_INVALID_SIZE);
    }
    CHECK(D5NextProtocol::ValidateReport(report, 810) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextProtocol::ValidateReport(nullptr, 809) == D5NEXT_INVALID_ARGUMENT);
    report[0] = 0x02;
    CHECK(D5NextProtocol::ValidateReport(report, 809) == D5NEXT_INVALID_REPORT_ID);
    report[0] = 0x03;

    for(unsigned int offset = 1; offset < 809; offset++)
    {
        report[offset] ^= 1;
        CHECK(D5NextProtocol::ValidateReport(report, 809) == D5NEXT_INVALID_CRC);
        report[offset] ^= 1;
    }
    unsigned char first = report[807];
    report[807] = report[808];
    report[808] = first;
    CHECK(D5NextProtocol::ValidateReport(report, 809) == D5NEXT_INVALID_CRC);

    for(unsigned int seed = 0; seed < 256; seed++)
    {
        MakeFixture(report, seed);
        CHECK(D5NextProtocol::ValidateReport(report, 809) == D5NEXT_OK);
    }
    std::puts("PASS CRC vectors, exact framing, and corrupted bytes");
}

static void TestColors()
{
    const unsigned int cases[][6] =
    {
        { 255,   0,   0,    0, 255, 255 },
        {   0, 255,   0,  512, 255, 255 },
        {   0,   0, 255, 1024, 255, 255 },
        { 255, 255, 255,    0,   0, 255 },
        {   0,   0,   0,    0,   0,   0 },
        { 255, 255,   0,  256, 255, 255 },
        {   0, 255, 255,  768, 255, 255 },
        { 255,   0, 255, 1280, 255, 255 },
        { 128, 128, 128,    0,   0, 128 },
        { 255,   0,   1, 1535, 255, 255 },
        { 255, 254, 254,    0,   1, 255 },
    };
    for(unsigned int index = 0; index < sizeof(cases) / sizeof(cases[0]); index++)
    {
        D5NextHSV lighting = D5NextProtocol::RGBToHSV((unsigned char)cases[index][0],
                                                          (unsigned char)cases[index][1],
                                                          (unsigned char)cases[index][2]);
        CHECK(lighting.hue == cases[index][3]);
        CHECK(lighting.saturation == cases[index][4]);
        CHECK(lighting.value == cases[index][5]);

    }

    /* Floating-point sector oracle across a deterministic RGB grid. */
    for(unsigned int red = 0; red <= 255; red += 17)
    for(unsigned int green = 0; green <= 255; green += 17)
    for(unsigned int blue = 0; blue <= 255; blue += 17)
    {
        double maximum = std::fmax(red, std::fmax(green, blue));
        double minimum = std::fmin(red, std::fmin(green, blue));
        double delta = maximum - minimum;
        double hue = 0;
        if(delta != 0)
        {
            if(maximum == red)
            {
                hue = ((double)green - blue) / delta;
            }
            else if(maximum == green)
            {
                hue = 2 + ((double)blue - red) / delta;
            }
            else
            {
                hue = 4 + ((double)red - green) / delta;
            }
            if(hue < 0)
            {
                hue += 6;
            }
        }
        D5NextHSV actual = D5NextProtocol::RGBToHSV(red, green, blue);
        CHECK(actual.hue == (unsigned int)std::floor(hue * 256 + 0.5) % 1536);
        CHECK(actual.saturation == (maximum == 0 ? 0 : (unsigned int)std::floor(delta * 255 / maximum + 0.5)));
        CHECK(actual.value == maximum);
    }
    std::puts("PASS RGB to HSV, primaries, grayscale, rounding, and hue wrap");
}

/*---------------------------------------------------------*\
| HID symbols are implemented here, with no OS I/O or       |
| dynamic HID library. Even accidental discovery cannot     |
| reach a real device from this executable.                 |
\*---------------------------------------------------------*/
struct hid_device_
{
    unsigned char report[809];
    int read_result = 809;
    int write_result = 809;
    unsigned int reads = 0;
    unsigned int writes = 0;
    unsigned int closes = 0;
    std::atomic<bool> in_transaction{ false };
    std::atomic<bool> overlap{ false };
};

static std::atomic<unsigned int> mock_failures{ 0 };
static hid_device_* discoverable_device = nullptr;
static unsigned int mock_opens = 0;

hid_device* hid_open_path(const char* path)
{
    mock_opens++;
    if(path == nullptr || strcmp(path, "/offline/d5-next") != 0)
    {
        return nullptr;
    }
    return discoverable_device;
}

int hid_get_feature_report(hid_device* dev, unsigned char* data, std::size_t length)
{
    if(length != 809 || data[0] != 3)
    {
        mock_failures++;
    }
    if(dev->in_transaction.exchange(true))
    {
        dev->overlap = true;
    }
    dev->reads++;
    std::this_thread::yield();
    memcpy(data, dev->report, length);
    if(dev->read_result != 809)
    {
        dev->in_transaction = false;
    }
    return dev->read_result;
}

int hid_send_feature_report(hid_device* dev, const unsigned char* data, std::size_t length)
{
    if(length != 809 || data[0] != 3 || !dev->in_transaction.load())
    {
        mock_failures++;
    }
    dev->writes++;
    if(dev->write_result == 809)
    {
        memcpy(dev->report, data, 809);
    }
    dev->in_transaction = false;
    return dev->write_result;
}

void hid_close(hid_device* dev)
{
    dev->closes++;
}

static void TestMockedController()
{
    hid_device_ fake;
    MakeFixture(fake.report);
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&fake));
        CHECK(controller.SetLighting(1, 0xFF, 255) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(controller.SetOff() == D5NEXT_UNSUPPORTED_LAYOUT);
        D5NextRGBpxPatch blocked = {};
        blocked.kind = D5NEXT_PATCH_MASTER;
        blocked.group = 6;
        blocked.expected_mode = 1;
        blocked.value = 255;
        CHECK(controller.ApplyRGBpxPatch(blocked) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(fake.reads == 0 && fake.writes == 0);
    }
    CHECK(fake.closes == 1);
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&fake), D5NEXT_LAYOUT_DOCUMENTED_809);
        CHECK(fake.reads == 0 && fake.writes == 0);
        unsigned char original[809];
        memcpy(original, fake.report, 809);
        CHECK(controller.SetLighting(1, 0x000000FF, 128) == D5NEXT_OK);
        CHECK(fake.report[237] == 128 && fake.report[706] == 0 && fake.report[707] == 0);
        CHECK(fake.report[708] == 255);
        CheckPreservation(original, fake.report);
        CHECK(controller.SetLighting(1, 0x000000FF, 128) == D5NEXT_UNCHANGED);
        CHECK(fake.reads == 2 && fake.writes == 1);
        /* No SET on no-op leaves the mock's observation window open. */
        fake.in_transaction = false;

        /* Concurrent external config change is preserved by a fresh read. */
        fake.report[500] ^= 0x80;
        FixCRC(fake.report);
        CHECK(controller.SetLighting(2, 0x00FF0000, 255) == D5NEXT_UNSUPPORTED_TRANSITION);
        fake.in_transaction = false;
        CHECK(controller.SetLighting(1, 0x00FF0000, 255) == D5NEXT_OK);
        CHECK(fake.report[500] == (unsigned char)(original[500] ^ 0x80));
        CHECK(fake.report[663] == 1 && fake.report[706] == 4 && fake.report[707] == 0);
        unsigned char before_off[809];
        memcpy(before_off, fake.report, 809);
        CHECK(controller.SetOff() == D5NEXT_OK);
        CHECK(fake.report[239] == 2);
        for(unsigned int offset = 0; offset < 807; offset++)
        {
            if(offset != 239)
            {
                CHECK(fake.report[offset] == before_off[offset]);
            }
        }
        CHECK(controller.SetLighting(3, 0, 0) == D5NEXT_UNSUPPORTED_TRANSITION);
        fake.in_transaction = false;
        /* An independent synthetic state, not a driver-generated preset. */
        fake.report[663] = 3;
        FixCRC(fake.report);
        CHECK(controller.SetLighting(3, 0, 0) == D5NEXT_OK);
        CHECK(fake.report[237] == 0 && fake.report[239] == 0 && fake.report[663] == 3);
        CHECK(fake.report[706] == 4 && fake.report[707] == 0 && fake.report[708] == 255);
        CHECK(controller.SetLighting(0xFE, 0, 255) == D5NEXT_INVALID_LIGHTING);
        CHECK(controller.SetLighting(1, 0, 256) == D5NEXT_INVALID_LIGHTING);
        CHECK(controller.SetLighting(1, 0xFF000000, 255) == D5NEXT_INVALID_LIGHTING);
    }

    /* Invalid baseline and uncertain/short transfers never trigger a retry. */
    for(unsigned int scenario = 0; scenario < 9; scenario++)
    {
        hid_device_ broken;
        MakeFixture(broken.report);
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&broken), D5NEXT_LAYOUT_DOCUMENTED_809);
        switch(scenario)
        {
        case 0: broken.read_result = -1; break;
        case 1: broken.read_result = 808; break;
        case 2: broken.write_result = -1; break;
        case 3: broken.write_result = 808; break;
        case 4: broken.report[10] ^= 1; break;
        case 5: broken.report[0] = 2; break;
        case 6:
            broken.report[663] = 0xFF;
            FixCRC(broken.report);
            break;
        case 7: broken.read_result = 810; break;
        case 8: broken.write_result = 810; break;
        }
        D5NextResult expected = D5NEXT_IO_ERROR;
        if(scenario == 4)
        {
            expected = D5NEXT_INVALID_CRC;
        }
        if(scenario == 5)
        {
            expected = D5NEXT_INVALID_REPORT_ID;
        }
        if(scenario == 6)
        {
            expected = D5NEXT_UNSUPPORTED_TRANSITION;
        }
        CHECK(controller.SetLighting(1, 0xFF, 255) == expected);
        CHECK(broken.writes == (scenario == 2 || scenario == 3 || scenario == 8 ? 1U : 0U));
        if(scenario < 4 || scenario > 6)
        {
            CHECK(controller.SetOff() == D5NEXT_NOT_READY);
            CHECK(broken.reads == 1);
        }
    }

    /* Different colors guarantee each concurrent transaction includes SET. */
    hid_device_ concurrent;
    MakeFixture(concurrent.report);
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&concurrent), D5NEXT_LAYOUT_DOCUMENTED_809);
        std::atomic<unsigned int> errors{ 0 };
        std::thread first([&]()
        {
            for(unsigned int value = 1; value <= 100; value++)
            {
                if(controller.SetLighting(1, ToRGBColor(255, value, 0), 255) != D5NEXT_OK)
                {
                    errors++;
                }
            }
        });
        std::thread second([&]()
        {
            for(unsigned int value = 1; value <= 100; value++)
            {
                if(controller.SetLighting(1, ToRGBColor(0, value, 255), 255) != D5NEXT_OK)
                {
                    errors++;
                }
            }
        });
        first.join();
        second.join();
        CHECK(errors == 0);
        CHECK(concurrent.reads == 200 && concurrent.writes == 200);
        CHECK(!concurrent.overlap);
        CHECK(D5NextProtocol::ValidateReport(concurrent.report, 809) == D5NEXT_OK);
    }
    CHECK(mock_failures == 0);
    std::puts("PASS mocked transactions, no persistence, fresh reads, errors, and serialization");
}

static void TestMockedRGBpxPatch()
{
    hid_device_ fake;
    MakeFixture(fake.report);
    fake.report[663] = 0x13;
    FixCRC(fake.report);
    unsigned char original[809];
    memcpy(original, fake.report, 809);
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&fake), D5NEXT_LAYOUT_DOCUMENTED_809);
        D5NextRGBpxPatch patch = {};
        patch.group = 6;
        patch.expected_mode = 0x13;
        patch.kind = D5NEXT_PATCH_PARAMETER;
        patch.value = 44;
        CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_OK);
        CHECK(fake.reads == 1 && fake.writes == 1 && fake.report[682] == 0 && fake.report[683] == 44);
        for(unsigned int offset = 0; offset < 807; offset++)
        {
            CHECK(offset == 682 || offset == 683 || fake.report[offset] == original[offset]);
        }
        CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_UNCHANGED);
        CHECK(fake.reads == 2 && fake.writes == 1);
        fake.in_transaction = false;
        patch.group = 0;
        CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_UNSUPPORTED_OPERATION);
        CHECK(fake.writes == 1);
        fake.in_transaction = false;
        patch.group = 6;
        fake.report[2] = 4;
        FixCRC(fake.report);
        CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_UNSUPPORTED_STRUCTURE);
        CHECK(fake.writes == 1);
    }
    CHECK(fake.closes == 1);
    MakeFixture(fake.report);
    fake.in_transaction = false;
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("offline-mock", L"test-serial"),
                                                D5NextHIDHandle(&fake), D5NEXT_LAYOUT_DOCUMENTED_809);
        unsigned int writes = fake.writes;
        CHECK(controller.SetLighting(1, ToRGBColor(128, 0, 0), 255) == D5NEXT_UNSUPPORTED_OPERATION);
        CHECK(fake.writes == writes && fake.report[237] == 87 && fake.report[709] == 255);
    }
    std::puts("PASS RGBpx mock transaction, structure, external-group refusal, and master/V separation");
}

static void TestOwnership()
{
    auto first = D5NextOwnership::Acquire("/offline/first", L"pump-serial");
    CHECK(first != nullptr);
    CHECK(D5NextOwnership::Acquire("/offline/first", L"another-serial") == nullptr);
    CHECK(D5NextOwnership::Acquire("/offline/alias", L"pump-serial") == nullptr);
    auto different = D5NextOwnership::Acquire("/offline/second", L"other-pump");
    CHECK(different != nullptr);
    CHECK(D5NextOwnership::Acquire(nullptr, L"pump-serial") == nullptr);
    different.reset();
    CHECK(D5NextOwnership::Acquire("/offline/second", L"other-pump") != nullptr);
    first.reset();
    CHECK(D5NextOwnership::Acquire("/offline/alias", L"pump-serial") != nullptr);

    hid_device_ fake;
    MakeFixture(fake.report);
    {
        AquaComputerD5NextController controller(D5NextOwnership::Acquire("/offline/no-serial", nullptr),
                                                D5NextHIDHandle(&fake), D5NEXT_LAYOUT_DOCUMENTED_809);
        D5NextRGBpxPatch patch = {};
        patch.kind = D5NEXT_PATCH_MASTER;
        patch.group = 6;
        patch.expected_mode = 1;
        patch.value = 150;
        CHECK(controller.ApplyRGBpxPatch(patch) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(fake.reads == 0 && fake.writes == 0);
    }
    CHECK(fake.closes == 1);
    hid_device_ left, right;
    MakeFixture(left.report);
    MakeFixture(right.report);
    {
        AquaComputerD5NextController a(D5NextOwnership::Acquire("/offline/pump-a", L"pump-a"),
                                       D5NextHIDHandle(&left), D5NEXT_LAYOUT_DOCUMENTED_809);
        AquaComputerD5NextController b(D5NextOwnership::Acquire("/offline/pump-b", L"pump-b"),
                                       D5NextHIDHandle(&right), D5NEXT_LAYOUT_DOCUMENTED_809);
        D5NextRGBpxPatch patch_a = {}, patch_b = {};
        patch_a.kind = patch_b.kind = D5NEXT_PATCH_MASTER;
        patch_a.group = patch_b.group = 6;
        patch_a.expected_mode = patch_b.expected_mode = 1;
        patch_a.value = 150;
        patch_b.value = 200;
        D5NextResult result_a = D5NEXT_NOT_READY, result_b = D5NEXT_NOT_READY;
        std::thread one([&]() { result_a = a.ApplyRGBpxPatch(patch_a); });
        std::thread two([&]() { result_b = b.ApplyRGBpxPatch(patch_b); });
        one.join();
        two.join();
        CHECK(result_a == D5NEXT_OK && result_b == D5NEXT_OK);
        CHECK(left.report[237] == 150 && right.report[237] == 200);
        CHECK(left.reads == 1 && left.writes == 1 && right.reads == 1 && right.writes == 1);
        CHECK(!left.overlap && !right.overlap);
    }
    CHECK(left.closes == 1 && right.closes == 1);
    std::puts("PASS in-process path/serial reservations and missing-serial write gate");
}

DetectedControllers DetectAquaComputerD5NextControllers(hid_device_info*, const std::string&);

static void TestReadOnlyAdapter()
{
    hid_device_ fake;
    MakeFixture(fake.report);
    fake.report[663] = D5NEXT_EFFECT_RAINBOW;
    FixCRC(fake.report);
    {
        std::unique_ptr<D5NextOwnership> owner = D5NextOwnership::Acquire("offline-adapter", L"adapter-serial");
        auto native = std::unique_ptr<AquaComputerD5NextController>(
            new AquaComputerD5NextController(std::move(owner), D5NextHIDHandle(&fake),
                                             D5NEXT_LAYOUT_DOCUMENTED_809));
        D5NextRGBpxPatch change = {};
        change.group = 6;
        change.expected_mode = D5NEXT_EFFECT_RAINBOW;
        change.kind = D5NEXT_PATCH_MODE;
        change.value = D5NEXT_EFFECT_STATIC;
        CHECK(native->ApplyRGBpxPatch(change) == D5NEXT_UNSUPPORTED_TRANSITION);
        CHECK(fake.reads == 1 && fake.writes == 0);
        fake.in_transaction = false;
        RGBController_AquaComputerD5Next rgb(std::move(native));
        CHECK(rgb.GetDeviceType() == DEVICE_TYPE_COOLER);
        CHECK(rgb.GetZoneCount() == 1 && rgb.GetLEDCount() == 6);
        CHECK(rgb.GetModeCount() == 1 && rgb.GetModeName(0) == "Read only");
        CHECK(rgb.GetModeFlags(0) == 0 && rgb.GetModeColorsCount(0) == 0);
        CHECK(rgb.GetActiveMode() == 0);
        rgb.SetActiveMode(1); /* Static must not exist in the published modes. */
        CHECK(rgb.GetActiveMode() == 0);
        rgb.DeviceUpdateLEDs();
        rgb.DeviceUpdateSingleLED(0);
        rgb.DeviceUpdateZoneLEDs(0);
        rgb.DeviceUpdateMode();
        rgb.SetAllColors(0x00FFFFFF);
        rgb.UpdateLEDs();
        rgb.SaveMode();
        rgb.SetDeviceSpecificConfiguration({ { "write_enabled", true } });
        CHECK(fake.reads == 1 && fake.writes == 0);
    }
    CHECK(fake.closes == 1 && fake.writes == 0);
    std::puts("PASS Rainbow->Static refusal does not publish Static or send SET");
}

/*---------------------------------------------------------*\
| Model the existing ProcessPreDetection/ProcessCleanup     |
| lifecycle: destroy old results before a new discovery.    |
| The full app / manager background thread is not started.  |
\*---------------------------------------------------------*/
static void SimulatedScan(const HIDDeviceDetectorBlock& detector, hid_device_info* info,
                          DetectedControllers& devices)
{
    for(unsigned int index = 0; index < devices.size(); index++)
    {
        delete devices[index];
    }
    devices = detector.function(info, detector.name);
}

static void TestDetection()
{
    const std::vector<HIDDeviceDetectorBlock>& registrations = GetOfflineHIDDetectors();
    CHECK(registrations.size() == 1);
    const HIDDeviceDetectorBlock& detector = registrations[0];
    CHECK(detector.name == "Aqua Computer D5 NEXT");
    CHECK(detector.vid == 0x0C70 && detector.pid == 0xF00E && detector.interface == 1);
    CHECK(detector.usage_page == HID_USAGE_PAGE_ANY && detector.usage == HID_USAGE_ANY);
    CHECK(detector.enabled_by_default);

    hid_device_ fake;
    MakeFixture(fake.report);
    discoverable_device = &fake;
    hid_device_info info = {};
    char path[] = "/offline/d5-next";
    info.path = path;
    info.vendor_id = 0x0C70;
    info.product_id = 0xF00E;
    info.interface_number = 1;
    info.release_number = 0xFFFF;
    info.serial_number = const_cast<wchar_t*>(L"offline-pump-serial");

    DetectedControllers devices;
    SimulatedScan(detector, nullptr, devices);
    CHECK(devices.empty() && mock_opens == 0);
    for(unsigned int scan = 0; scan < 3; scan++)
    {
        SimulatedScan(detector, &info, devices);
        CHECK(devices.size() == 1);
        CHECK(devices[0]->GetName() == "Aqua Computer D5 NEXT");
        CHECK(devices[0]->GetLocation() == "HID: /offline/d5-next");
        CHECK(devices[0]->GetModeCount() == 1);
        CHECK(devices[0]->GetModeName(0) == "Read only");
        DetectedControllers duplicate = detector.function(&info, detector.name);
        CHECK(duplicate.empty() && mock_opens == scan + 1);
        CHECK(devices[0]->GetModeFlags(0) == 0);
        CHECK(devices[0]->GetModeColorMode(0) == MODE_COLORS_NONE);
        CHECK(fake.closes == scan && mock_opens == scan + 1);
        CHECK(fake.reads == 0 && fake.writes == 0);
        /* A profile/app callback during registration cannot bypass the
           unknown-firmware gate, and saving has no implementation. */
        devices[0]->UpdateZoneLEDs(0);
        devices[0]->UpdateSingleLED(0);
        devices[0]->SetAllColors(0x00FFFFFF);
        devices[0]->SetModeBrightness(0, 255);
        devices[0]->SetActiveMode(0);
        devices[0]->UpdateLEDs();
        devices[0]->SetCustomMode();
        devices[0]->SaveMode();
        CHECK(fake.reads == 0 && fake.writes == 0);
    }
    SimulatedScan(detector, nullptr, devices);
    CHECK(devices.empty() && fake.closes == 3);

    discoverable_device = nullptr;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    CHECK(fake.closes == 3);

    discoverable_device = &fake;
    info.interface_number = 0;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    info.interface_number = 2;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    info.interface_number = 1;
    info.product_id = 0xF012;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    info.product_id = 0xF00E;
    info.vendor_id = 0xFFFF;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    info.vendor_id = 0x0C70;
    info.path = nullptr;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);
    char empty_path[] = "";
    info.path = empty_path;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.empty() && mock_opens == 4);

    /* A later discovery after disappearance can reopen the same path. */
    info.path = path;
    SimulatedScan(detector, &info, devices);
    CHECK(devices.size() == 1 && mock_opens == 5);
    SimulatedScan(detector, nullptr, devices);
    CHECK(devices.empty() && fake.closes == 4);
    CHECK(fake.reads == 0 && fake.writes == 0);
    discoverable_device = nullptr;
    std::puts("PASS native registration, simulated scans, matching, permissions failure, and handle lifetime");
}

int main()
{
    TestCRCAndFraming();
    TestColors();
    TestMockedController();
    TestMockedRGBpxPatch();
    TestOwnership();
    TestReadOnlyAdapter();
    TestDetection();
    std::printf("PASS all D5 NEXT offline tests (%u checks); no hardware backend linked\n", checks);
    return EXIT_SUCCESS;
}
