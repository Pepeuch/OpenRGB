/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Actual read-only registration macro, fake HID backend and access syscall. */
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <unistd.h>
#include "OfflineDetection.h"
#include "D5NextOwnership.h"

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

struct hid_device_ { unsigned int marker; };
static hid_device_ fake = { 1 };
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
int hid_get_feature_report(hid_device*, unsigned char*, std::size_t)
{
    gets++;
    CHECK(false);
    return -1;
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
    DetectedControllers duplicate = detector.function(&info, detector.name);
    CHECK(duplicate.empty() && opens == 2);
    auto alias = D5NextOwnership::Acquire("/offline/alias", L"in-process-test-serial");
    CHECK(!alias);
    Destroy(first);
    CHECK(opens == 2 && closes == 1 && gets == 0 && sets == 0);

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
        CHECK(gets == 0 && sets == 0);
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
    CHECK(opens == closes + 1 && gets == 0 && sets == 0); /* one failed open */
    std::printf("PASS real read-only macro with fake HID: %u allocation sites, %u after open, %u checks; no GET/SET\n",
                baseline_allocations, post_open_failures, checks);
    return 0;
}
