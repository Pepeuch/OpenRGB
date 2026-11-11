/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Actual read-only registration macro, fake HID backend and access syscall. */
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <cwchar>
#include <thread>
#include <unistd.h>
#include "OfflineDetection.h"
#include "D5NextOwnership.h"
#include "D5NextRGBpx.h"
#include "AquaComputerD5NextController.h"

#if !defined(OPENRGB_D5NEXT_READ_ONLY_DETECTION) || defined(OPENRGB_D5NEXT_OFFLINE_TESTS)
#error Test requires the production read-only detector and no offline-test macro
#endif

static thread_local unsigned fail_at;
static thread_local unsigned allocation_count;
void* operator new(std::size_t size)
{
    if(fail_at != 0 && ++allocation_count == fail_at)
    {
        throw std::bad_alloc();
    }
    if(void* result = std::malloc(size == 0 ? 1 : size))
    {
        return result;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

static unsigned checks, opens, closes, gets, sets, access_checks;
static bool access_allowed = true;
static bool open_allowed = true;
static unsigned char report[809];
static int status_length = 162, feature_length = 809;
static unsigned int observed_firmware = 1027;
static bool serial_mismatch = false;
static unsigned char status_id = 1;
static void Baseline(unsigned int count = 13)
{
    std::memset(report, 0, sizeof(report));
    report[0] = 3;
    report[2] = 3;
    const unsigned int starts[] = {0, 13, 33, 48, 63, 78, 0, 0}, counts[] = {count, 20, 15, 15, 15, 12, 6, 6};
    for(unsigned int i = 0; i < 8; i++)
    {
        report[240 + 70 * i] = i < 6 ? 0 : 1;
        report[241 + 70 * i] = starts[i];
        report[242 + 70 * i] = counts[i];
        report[243 + 70 * i] = 2;
    }
    auto crc = D5NextProtocol::CalculateCRC(report + 1, 806);
    report[807] = (unsigned char)(crc >> 8);
    report[808] = (unsigned char)crc;
    status_length = 162;
    feature_length = 809;
    observed_firmware = 1027;
    serial_mismatch = false;
    status_id = 1;
}
static void FixCRC()
{
    auto crc = D5NextProtocol::CalculateCRC(report + 1, 806);
    report[807] = (unsigned char)(crc >> 8);
    report[808] = (unsigned char)crc;
}
#define CHECK(expression) Check((expression), #expression, __LINE__)
static void Check(bool result, const char* expression, int line)
{
    checks++;
    if(!result)
    {
        std::fprintf(stderr, "FAIL read-only detection line %d: %s\n", line, expression);
        std::exit(1);
    }
}

struct hid_device_
{
    unsigned int marker;
};
static hid_device_ fake = {1};
hid_device* hid_open_path(const char* path)
{
    CHECK(std::strcmp(path, "/offline/d5-next/failure-injection") == 0);
    opens++;
    return open_allowed ? &fake : nullptr;
}
void hid_close(hid_device* handle)
{
    CHECK(handle == &fake);
    closes++;
}
int hid_get_feature_report(hid_device* handle, unsigned char* bytes, std::size_t size)
{
    CHECK(handle == &fake && size == 809 && bytes[0] == 3);
    gets++;
    std::memcpy(bytes, report, 809);
    return feature_length;
}
int hid_read_timeout(hid_device* handle, unsigned char* bytes, std::size_t size, int timeout)
{
    CHECK(handle == &fake && size == 163 && timeout == 2000);
    std::memset(bytes, 0, size);
    bytes[0] = status_id;
    bytes[13] = (unsigned char)(observed_firmware >> 8);
    bytes[14] = (unsigned char)observed_firmware;
    return status_length;
}
int hid_get_serial_number_string(hid_device* handle, wchar_t* serial, std::size_t count)
{
    CHECK(handle == &fake && count == 256);
    std::wcscpy(serial, serial_mismatch ? L"wrong" : L"in-process-test-serial");
    return 0;
}
int hid_send_feature_report(hid_device*, const unsigned char*, std::size_t)
{
    sets++;
    CHECK(false);
    return -1;
}
extern "C" int __wrap_access(const char* path, int mode)
{
    CHECK(std::strcmp(path, "/offline/d5-next/failure-injection") == 0);
    CHECK(mode == (R_OK | W_OK));
    access_checks++;
    errno = access_allowed ? 0 : EACCES;
    return access_allowed ? 0 : -1;
}

static void Destroy(DetectedControllers& controllers)
{
    for(RGBController* controller : controllers)
    {
        delete controller;
    }
    controllers.clear();
}

int main()
{
    Baseline();
    const auto& detectors = GetOfflineHIDDetectors();
    CHECK(detectors.size() == 1);
    const auto& detector = detectors[0];
    CHECK(detector.vid == 0x0C70 && detector.pid == 0xF00E && detector.interface == 1);
    hid_device_info info = {};
    char path[] = "/offline/d5-next/failure-injection";
    info.path = path;
    info.vendor_id = 0x0C70;
    info.product_id = 0xF00E;
    info.interface_number = 1;
    info.serial_number = const_cast<wchar_t*>(L"in-process-test-serial");
    info.release_number = 1027; /* USB release is never write authorization. */

    access_allowed = false;
    CHECK(detector.function(&info, detector.name).empty());
    CHECK(opens == 0 && access_checks == 1);
    access_allowed = true;
    info.interface_number = 2;
    CHECK(detector.function(&info, detector.name).empty());
    CHECK(opens == 0);
    info.interface_number = 1;

    open_allowed = false;
    CHECK(detector.function(&info, detector.name).empty());
    CHECK(opens == 1 && closes == 0);
    CHECK(D5NextOwnership::Acquire(path, L"in-process-test-serial") != nullptr);
    open_allowed = true;

    allocation_count = 0;
    fail_at = UINT_MAX;
    DetectedControllers first = detector.function(&info, detector.name);
    unsigned baseline_allocations = allocation_count;
    fail_at = 0;
    CHECK(first.size() == 1 && baseline_allocations > 5);
    CHECK(first[0]->GetModeCount() == 1 && first[0]->GetModeName(0) == "Read only");
    CHECK(first[0]->GetZoneCount() == 2 && first[0]->GetLEDCount() == 19);
    CHECK(first[0]->GetZoneName(0) == "Pump RGB" && first[0]->GetZoneLEDsCount(0) == 6);
    CHECK(first[0]->GetZoneName(1) == "External RGBpx" && first[0]->GetZoneLEDsCount(1) == 13);
    CHECK(first[0]->GetVersion() == "1027");
    const auto profile = RGBController::GetDeviceDescriptionJSON(first[0]);
    CHECK(profile["leds"].size() == 19 && profile["colors"].size() == 19);
    CHECK(profile["zones"].size() == 2 && profile["modes"].size() == 1);
    CHECK(profile["zones"][0]["leds_count"] == 6 && profile["zones"][1]["leds_count"] == 13);
    CHECK(!first[0]->SupportsPerZoneModes());
    CHECK(first[0]->GetModeFlags(0) == 0 && first[0]->GetModeColorMode(0) == MODE_COLORS_NONE);
    CHECK(first[0]->GetZoneStartIndex(0) == 0 && first[0]->GetZoneStartIndex(1) == 6);
    for(unsigned int zone = 0; zone < 2; zone++)
    {
        unsigned int count = zone == 0 ? 6 : 13;
        CHECK(first[0]->GetZoneLEDsMin(zone) == count && first[0]->GetZoneLEDsMax(zone) == count);
        CHECK(first[0]->GetZoneFlags(zone) == 0 && first[0]->GetZoneType(zone) == ZONE_TYPE_LINEAR);
        CHECK(first[0]->GetZoneColorsPointer(zone) == first[0]->GetColorsPointer() + (zone == 0 ? 0 : 6));
        CHECK(first[0]->GetZoneModeCount(zone) == 0 && first[0]->GetZoneActiveMode(zone) == -1);
        first[0]->ResizeZone(zone, 99);
        CHECK(first[0]->GetZoneLEDsCount(zone) == count);
    }
    unsigned int baseline_gets = gets;
    first[0]->SetAllColors(0xFFFFFF);
    first[0]->SetActiveMode(0);
    first[0]->UpdateZoneLEDs(1);
    first[0]->SaveMode();
    first[0]->SetDeviceSpecificConfiguration({{"write_authorized", true}, {"firmware", 1027}});
    CHECK(gets == baseline_gets && sets == 0);
    DetectedControllers duplicate = detector.function(&info, detector.name);
    CHECK(duplicate.empty() && opens == 2);
    auto alias = D5NextOwnership::Acquire("/offline/alias", L"in-process-test-serial");
    CHECK(!alias);
    Destroy(first);
    CHECK(opens == 2 && closes == 1 && gets == 1 && sets == 0);

    unsigned post_open_failures = 0;
    for(unsigned ordinal = 1; ordinal <= baseline_allocations; ordinal++)
    {
        const unsigned before_opens = opens;
        const unsigned before_closes = closes;
        allocation_count = 0;
        fail_at = ordinal;
        bool threw = false;
        try
        {
            DetectedControllers found = detector.function(&info, detector.name);
            Destroy(found);
        }
        catch(const std::bad_alloc&)
        {
            threw = true;
        }
        fail_at = 0;
        CHECK(threw);
        CHECK(opens - before_opens == closes - before_closes);
        CHECK(sets == 0);
        if(opens != before_opens)
        {
            post_open_failures++;
        }
        auto released = D5NextOwnership::Acquire(path, L"in-process-test-serial");
        CHECK(released != nullptr);
    }
    CHECK(post_open_failures > 4);
    DetectedControllers again = detector.function(&info, detector.name);
    CHECK(again.size() == 1);
    Destroy(again);
    CHECK(opens == closes + 1 && sets == 0); /* one failed open */
    for(unsigned int count : {1U, 7U, 13U})
    {
        Baseline(count);
        auto found = detector.function(&info, detector.name);
        CHECK(found.size() == 1 && found[0]->GetLEDCount() == 6 + count);
        CHECK(found[0]->GetZoneLEDsCount(1) == count);
        Destroy(found);
    }
    Baseline(0);
    report[243] = 0;
    FixCRC();
    auto pump_only = detector.function(&info, detector.name);
    CHECK(pump_only.size() == 1 && pump_only[0]->GetZoneCount() == 1 && pump_only[0]->GetLEDCount() == 6);
    Destroy(pump_only);
    for(unsigned int scenario = 0; scenario < 18; scenario++)
    {
        Baseline();
        if(scenario == 0)
            observed_firmware = 1028;
        if(scenario == 1)
            status_length = 0;
        if(scenario == 2)
            status_length = 161;
        if(scenario == 3)
            status_id = 2;
        if(scenario == 4)
            feature_length = 808;
        if(scenario == 5)
            serial_mismatch = true;
        if(scenario == 6)
            report[100] ^= 1;
        if(scenario == 7)
        {
            report[2] = 4;
            FixCRC();
        }
        if(scenario == 8)
        {
            report[242] = 14;
            FixCRC();
        } // overlaps g1
        if(scenario == 9)
        {
            report[662] = 7;
            FixCRC();
        }
        if(scenario == 10)
        {
            report[242] = 91;
            FixCRC();
        }
        if(scenario == 11)
        {
            report[243] = 6;
            FixCRC();
        } // unknown mode
        if(scenario == 12)
            status_length = -1;
        if(scenario == 13)
            status_length = 163;
        if(scenario == 14)
            feature_length = 810;
        if(scenario == 15)
            feature_length = -1;
        if(scenario == 16)
            report[0] = 4;
        if(scenario == 17)
        {
            report[1] = 1;
            FixCRC();
        }
        unsigned int old_gets = gets, old_closes = closes;
        CHECK(detector.function(&info, detector.name).empty());
        CHECK(closes == old_closes + 1 && sets == 0);
        if(scenario <= 3 || scenario == 5 || scenario == 12 || scenario == 13)
            CHECK(gets == old_gets);
        CHECK(D5NextOwnership::Acquire(path, L"in-process-test-serial") != nullptr);
    }
    Baseline();
    {
        AquaComputerD5NextController native(D5NextOwnership::Acquire(path, info.serial_number),
                                            D5NextHIDHandle(hid_open_path(path)));
        D5NextRGBpxConfiguration snapshot = {};
        snapshot.master = 73;
        CHECK(!native.GetConfiguration(snapshot) && snapshot.master == 73);
        D5NextResult results[2];
        std::thread first_read([&]() { results[0] = native.ReadConfiguration(); });
        std::thread second_read([&]() { results[1] = native.ReadConfiguration(); });
        first_read.join();
        second_read.join();
        CHECK((results[0] == D5NEXT_OK && results[1] == D5NEXT_NOT_READY)
           || (results[1] == D5NEXT_OK && results[0] == D5NEXT_NOT_READY));
        CHECK(native.GetConfiguration(snapshot) && native.GetFirmware() == 1027);
        const unsigned int before_gets = gets;
        CHECK(native.ReadConfiguration() == D5NEXT_NOT_READY);
        CHECK(native.SetLighting(2, 0xFF0000, 255) == D5NEXT_UNSUPPORTED_LAYOUT);
        CHECK(native.SetOff() == D5NEXT_UNSUPPORTED_LAYOUT);
        for(unsigned int group : {0U, 6U})
        {
            for(unsigned int kind = D5NEXT_PATCH_MASTER; kind <= D5NEXT_PATCH_MODE; kind++)
            {
                D5NextRGBpxPatch patch = {};
                patch.group = group;
                patch.expected_mode = 2;
                patch.kind = static_cast<D5NextPatchKind>(kind);
                CHECK(native.ApplyRGBpxPatch(patch) == D5NEXT_UNSUPPORTED_LAYOUT);
            }
        }
        CHECK(gets == before_gets && sets == 0);
    }
    CHECK(opens == closes + 1);
    std::printf("PASS real read-only macro with fake HID: %u allocation sites, %u after open, %u checks; no SET\n",
                baseline_allocations, post_open_failures, checks);
    return 0;
}
