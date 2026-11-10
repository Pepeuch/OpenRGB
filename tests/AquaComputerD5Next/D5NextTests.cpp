/*---------------------------------------------------------*\
| D5NextTests.cpp                                           |
|                                                           |
|   Synthetic reports and in-process HID mocks only         |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
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
    report[237] = 87;
    report[239] = 0x00;
    report[663] = 0x01;
    report[706] = 0x01;
    report[707] = 0xFF;
    report[708] = 155;
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
    unsigned char original[809];
    unsigned char modified[809];
    MakeFixture(original);
    for(unsigned int index = 0; index < sizeof(cases) / sizeof(cases[0]); index++)
    {
        D5NextLighting lighting = D5NextProtocol::RGBToHSV((unsigned char)cases[index][0],
                                                          (unsigned char)cases[index][1],
                                                          (unsigned char)cases[index][2]);
        CHECK(lighting.hue == cases[index][3]);
        CHECK(lighting.saturation == cases[index][4]);
        CHECK(lighting.brightness == cases[index][5]);
        CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                                  lighting, modified, 809) == D5NEXT_OK);
        CHECK(modified[706] == (lighting.hue >> 8));
        CHECK(modified[707] == (lighting.hue & 0xFF));
        CHECK(modified[708] == lighting.saturation);
        CHECK(modified[237] == lighting.brightness);
        CHECK(modified[239] == 0x00);
        CHECK(modified[663] == 0x01);
        CheckPreservation(original, modified);
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
        D5NextLighting actual = D5NextProtocol::RGBToHSV(red, green, blue);
        CHECK(actual.hue == (unsigned int)std::floor(hue * 256 + 0.5) % 1536);
        CHECK(actual.saturation == (maximum == 0 ? 0 : (unsigned int)std::floor(delta * 255 / maximum + 0.5)));
        CHECK(actual.brightness == maximum);
    }
    std::puts("PASS RGB to HSV, primaries, grayscale, rounding, and hue wrap");
}

static void TestMutationsAndRejections()
{
    unsigned char original[809];
    unsigned char snapshot[809];
    unsigned char modified[809];
    MakeFixture(original);
    memcpy(snapshot, original, 809);
    D5NextLighting lighting;
    CHECK(D5NextProtocol::Decode(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809, lighting) == D5NEXT_OK);
    CHECK(lighting.hue == 511 && lighting.saturation == 155 && lighting.brightness == 87);
    CHECK(lighting.enabled == 0 && lighting.effect == 1);
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_UNCHANGED);
    CHECK(memcmp(original, modified, 809) == 0);

    lighting.brightness = 255;
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_OK);
    for(unsigned int offset = 0; offset < 807; offset++)
    {
        if(offset != 237)
        {
            CHECK(original[offset] == modified[offset]);
        }
    }

    /* All brightness/saturation boundaries, including hue endpoint. */
    const unsigned int limits[] = { 0, 1, 254, 255 };
    for(unsigned int saturation = 0; saturation < 4; saturation++)
    for(unsigned int brightness = 0; brightness < 4; brightness++)
    {
        lighting.hue        = 1535;
        lighting.saturation = limits[saturation];
        lighting.brightness = limits[brightness];
        CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                                  lighting, modified, 809) == D5NEXT_OK);
        CHECK(modified[706] == 5 && modified[707] == 255);
        CHECK(modified[708] == limits[saturation] && modified[237] == limits[brightness]);
        CheckPreservation(original, modified);
    }
    CHECK(memcmp(original, snapshot, 809) == 0);

    /* Repeating a color, including in-place/overlapping output. */
    lighting = D5NextProtocol::RGBToHSV(12, 143, 250);
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_OK);
    CHECK(D5NextProtocol::BuildLightingReport(modified, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_UNCHANGED);
    unsigned char overlap[810];
    memcpy(overlap, original, 809);
    CHECK(D5NextProtocol::BuildLightingReport(overlap, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, overlap + 1, 809) == D5NEXT_OK);
    CHECK(memcmp(overlap + 1, modified, 809) == 0);

    /* Independent allowlist audit: even a freshly recomputed CRC cannot
       authorize changes to any other config byte. */
    for(unsigned int offset = 1; offset < 807; offset++)
    {
        memcpy(modified, original, 809);
        if(!AllowedOffset(offset))
        {
            modified[offset] ^= 1;
            FixCRC(modified);
            CHECK(D5NextProtocol::ValidateLightingChange(original, 809, modified, 809,
                                                         D5NEXT_LAYOUT_DOCUMENTED_809) == D5NEXT_UNEXPECTED_CHANGE);
        }
        CHECK(D5NextProtocol::IsLightingOffset(offset) == (AllowedOffset(offset) && offset < 807));
    }

    /* Invalid requests leave caller-owned output unchanged. */
    for(unsigned int field = 0; field < 5; field++)
    {
        lighting = D5NextProtocol::RGBToHSV(255, 0, 0);
        switch(field)
        {
        case 0: lighting.hue = 1536; break;
        case 1: lighting.saturation = 256; break;
        case 2: lighting.brightness = 256; break;
        case 3: lighting.enabled = 1; break;
        case 4: lighting.effect = 0xFF; break;
        }
        memset(modified, 0xA5, 809);
        CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                                  lighting, modified, 809) == D5NEXT_INVALID_LIGHTING);
        for(unsigned int offset = 0; offset < 809; offset++)
        {
            CHECK(modified[offset] == 0xA5);
        }
    }
    lighting = D5NextProtocol::RGBToHSV(0, 0, 255);
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_UNKNOWN,
                                              lighting, modified, 809) == D5NEXT_UNSUPPORTED_LAYOUT);
    CHECK(D5NextProtocol::Decode(original, 809, (D5NextLayout)99, lighting) == D5NEXT_UNSUPPORTED_LAYOUT);
    CHECK(D5NextProtocol::BuildLightingReport(original, 808, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 808) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextProtocol::BuildLightingReport(nullptr, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, modified, 809) == D5NEXT_INVALID_ARGUMENT);
    CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                              lighting, nullptr, 809) == D5NEXT_INVALID_ARGUMENT);

    const unsigned int invalid_offsets[] = { 239, 663, 706 };
    const unsigned char invalid_values[] = { 1, 0xFE, 6 };
    for(unsigned int index = 0; index < 3; index++)
    {
        MakeFixture(original);
        original[invalid_offsets[index]] = invalid_values[index];
        FixCRC(original);
        CHECK(D5NextProtocol::ValidateReport(original, 809) == D5NEXT_OK);
        CHECK(D5NextProtocol::BuildLightingReport(original, 809, D5NEXT_LAYOUT_DOCUMENTED_809,
                                                  lighting, modified, 809) == D5NEXT_INVALID_LIGHTING);
    }
    std::puts("PASS decoding, bounds, preservation, idempotence, and conservative rejections");
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
        AquaComputerD5NextController controller(&fake, "offline-mock");
        CHECK(controller.SetLighting(1, 0xFF, 255) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(controller.SetOff() == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(fake.reads == 0 && fake.writes == 0);
    }
    CHECK(fake.closes == 1);
    {
        AquaComputerD5NextController controller(nullptr, nullptr, D5NEXT_LAYOUT_DOCUMENTED_809);
        CHECK(controller.SetOff() == D5NEXT_NOT_READY);
    }
    {
        AquaComputerD5NextController controller(&fake, "offline-mock", D5NEXT_LAYOUT_DOCUMENTED_809);
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
        CHECK(controller.SetLighting(2, 0x00FF0000, 255) == D5NEXT_OK);
        CHECK(fake.report[500] == (unsigned char)(original[500] ^ 0x80));
        CHECK(fake.report[663] == 2 && fake.report[706] == 4 && fake.report[707] == 0);
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
        AquaComputerD5NextController controller(&broken, "offline-mock", D5NEXT_LAYOUT_DOCUMENTED_809);
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
            expected = D5NEXT_INVALID_LIGHTING;
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
        AquaComputerD5NextController controller(&concurrent, "offline-mock", D5NEXT_LAYOUT_DOCUMENTED_809);
        std::atomic<unsigned int> errors{ 0 };
        std::thread first([&]()
        {
            for(unsigned int value = 1; value <= 100; value++)
            {
                if(controller.SetLighting(1, value, 255) != D5NEXT_OK)
                {
                    errors++;
                }
            }
        });
        std::thread second([&]()
        {
            for(unsigned int value = 1; value <= 100; value++)
            {
                if(controller.SetLighting(1, value << 16, 255) != D5NEXT_OK)
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

DetectedControllers DetectAquaComputerD5NextControllers(hid_device_info*, const std::string&);

class InspectableD5Next : public RGBController_AquaComputerD5Next
{
public:
    using RGBController_AquaComputerD5Next::RGBController_AquaComputerD5Next;

    mode GetTestMode(unsigned int index)
    {
        return modes[index];
    }

    /* Set state without queuing work: exercise synchronous public RGB
       callbacks without depending on the base worker's startup timing. */
    void SelectTestMode(int index)
    {
        std::lock_guard<std::shared_mutex> lock(AccessMutex);
        active_mode = index;
    }
};

static void CountCallback(void* argument, unsigned int reason, void* /*controller*/)
{
    unsigned int* counts = (unsigned int*)argument;
    if(reason <= RGBCONTROLLER_UPDATE_REASON_SAVEMODE)
    {
        counts[reason]++;
    }
}

static void TestRGBControllerAPI()
{
    CHECK(DetectAquaComputerD5NextControllers(nullptr, "offline").empty());
    hid_device_ fake;
    MakeFixture(fake.report);
    {
        InspectableD5Next rgb(new AquaComputerD5NextController(&fake, "offline-mock",
                                                              D5NEXT_LAYOUT_DOCUMENTED_809));
        CHECK(fake.reads == 0 && fake.writes == 0);
        CHECK(rgb.GetDeviceType() == DEVICE_TYPE_COOLER);
        CHECK(rgb.GetZoneCount() == 1 && rgb.GetLEDCount() == 1);
        zone global = rgb.GetZone(0);
        CHECK(global.type == ZONE_TYPE_SINGLE);
        CHECK(global.leds_min == 1 && global.leds_max == 1 && global.leds_count == 1);
        CHECK(global.matrix_map.map.empty() && global.flags == 0);
        CHECK(rgb.GetModeCount() == 4);
        CHECK(!rgb.SupportsPerZoneModes());
        for(unsigned int index = 0; index < 4; index++)
        {
            mode current = rgb.GetTestMode(index);
            CHECK((current.flags & (MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_MANUAL_SAVE | MODE_FLAG_AUTOMATIC_SAVE
                                  | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_DIRECTION_LR)) == 0);
            CHECK(current.name != "Direct" && current.name != "Custom");
            if(index == 1 || index == 2)
            {
                CHECK(current.color_mode == MODE_COLORS_MODE_SPECIFIC);
                CHECK(current.colors_min == 1 && current.colors_max == 1 && current.colors.size() == 1);
            }
            else
            {
                CHECK(current.color_mode == MODE_COLORS_NONE);
            }
        }
        unsigned int callback_counts[3] = { 0, 0, 0 };
        rgb.RegisterUpdateCallback(CountCallback, callback_counts);
        rgb.SetModeColor(1, 0, ToRGBColor(0, 255, 0));
        rgb.SetModeBrightness(1, 64);
        rgb.SelectTestMode(1);
        rgb.UpdateZoneLEDs(0);
        CHECK(fake.report[237] == 64 && fake.report[706] == 2 && fake.report[707] == 0);
        CHECK(fake.report[663] == 1);
        fake.in_transaction = false;
        unsigned int writes = fake.writes;
        rgb.DeviceUpdateSingleLED(9);
        rgb.DeviceUpdateZoneLEDs(9);
        CHECK(fake.writes == writes);
        rgb.SaveMode();
        CHECK(fake.writes == writes);
        rgb.SelectTestMode(0);
        rgb.UpdateSingleLED(0);
        CHECK(fake.report[239] == 2);
        CHECK(callback_counts[RGBCONTROLLER_UPDATE_REASON_UPDATELEDS] == 2);
        CHECK(callback_counts[RGBCONTROLLER_UPDATE_REASON_SAVEMODE] == 1);
        rgb.UnregisterUpdateCallback(callback_counts);
    }
    CHECK(fake.closes == 1);
    CHECK(mock_failures == 0);
    std::puts("PASS native RGBController modes, zone model, and callbacks");
}

static void TestReadOnlyAdapter()
{
    hid_device_ fake;
    MakeFixture(fake.report);
    {
        /* A known test layout deliberately removes the controller's
           independent layout barrier: the adapter must still send nothing. */
        InspectableD5Next rgb(new AquaComputerD5NextController(&fake, "offline-mock",
                                                              D5NEXT_LAYOUT_DOCUMENTED_809), true);
        CHECK(rgb.GetModeCount() == 1 && rgb.GetModeName(0) == "Read only");
        CHECK(rgb.GetModeFlags(0) == 0 && rgb.GetModeColorsCount(0) == 0);
        rgb.DeviceUpdateLEDs();
        rgb.DeviceUpdateSingleLED(0);
        rgb.DeviceUpdateZoneLEDs(0);
        rgb.DeviceUpdateMode();
        rgb.SetAllColors(0x00FFFFFF);
        rgb.SetActiveMode(0);
        rgb.UpdateLEDs();
        rgb.SaveMode();
        CHECK(fake.reads == 0 && fake.writes == 0);
    }
    CHECK(fake.closes == 1);
    CHECK(fake.reads == 0 && fake.writes == 0);
    std::puts("PASS read-only adapter rejects all updates even with a known test layout");
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
    TestMutationsAndRejections();
    TestMockedController();
    TestRGBControllerAPI();
    TestReadOnlyAdapter();
    TestDetection();
    std::printf("PASS all D5 NEXT offline tests (%u checks); no hardware backend linked\n", checks);
    return EXIT_SUCCESS;
}
