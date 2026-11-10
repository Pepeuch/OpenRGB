/*---------------------------------------------------------*\
| PinkTests.cpp                                             |
|                                                           |
|   Synthetic pink plans and mocked one-shot transactions   |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <thread>
#include "D5NextPinkTest.h"

#define CHECK(condition) Check((condition), #condition, __LINE__)
static unsigned int checks = 0;

static void Check(bool condition, const char* expression, int line)
{
    checks++;
    if(!condition)
    {
        std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
        std::exit(1);
    }
}

static void FixCRC(unsigned char* report)
{
    unsigned short crc = D5NextProtocol::CalculateCRC(report + 1, 806);
    report[807] = (unsigned char)(crc >> 8);
    report[808] = (unsigned char)crc;
}

static void Fixture(unsigned char* report, unsigned int seed)
{
    for(unsigned int offset = 0; offset < 809; offset++)
    {
        report[offset] = (unsigned char)(offset * 31 + seed);
    }
    report[0] = 3;
    report[237] = 255;
    report[239] = 0;
    report[663] = 3;
    report[706] = 0;
    report[707] = 0;
    report[708] = 255;
    FixCRC(report);
}

static void TestPlans()
{
    const unsigned int offsets[] = { 663, 706, 707, 708, 807, 808 };
    unsigned char original[809];
    unsigned char candidate[809];
    unsigned char snapshot[809];
    for(unsigned int seed = 0; seed < 256; seed++)
    {
        Fixture(original, seed);
        memcpy(snapshot, original, 809);
        D5NextPinkAudit audit;
        CHECK(D5NextPinkTest::BuildPlan(original, 809, candidate, 809, audit) == D5NEXT_OK);
        CHECK(memcmp(original, snapshot, 809) == 0);
        CHECK(audit.changed_count == 6);
        CHECK(audit.target.hue == 1408 && audit.target.saturation == 150 && audit.target.brightness == 255);
        CHECK(candidate[663] == 1 && candidate[706] == 5 && candidate[707] == 128 && candidate[708] == 150);
        CHECK((audit.initial_crc ^ audit.target_crc) == 0x17B1);
        for(unsigned int index = 0; index < 6; index++)
        {
            CHECK(audit.offsets[index] == offsets[index]);
            CHECK(audit.before[index] == original[offsets[index]] && audit.after[index] == candidate[offsets[index]]);
        }
        for(unsigned int offset = 0; offset < 809; offset++)
        {
            if(offset != 663 && offset != 706 && offset != 707 && offset != 708 && offset < 807)
            {
                CHECK(original[offset] == candidate[offset]);
            }
        }
    }
    CHECK((0xEFD1 ^ 0x17B1) == 0xF860); /* Conditional user-summary prediction, not a captured fixture. */

    for(unsigned int offset = 0; offset < 809; offset++)
    {
        Fixture(original, 19);
        D5NextPinkAudit audit;
        CHECK(D5NextPinkTest::BuildPlan(original, 809, candidate, 809, audit) == D5NEXT_OK);
        candidate[offset] ^= 1;
        if(offset < 807)
        {
            FixCRC(candidate);
        }
        CHECK(D5NextPinkTest::AuditPlan(original, 809, candidate, 809, audit) != D5NEXT_OK);
    }

    const unsigned int unknown_fields[] = { 237, 239, 663, 706, 708 };
    for(unsigned int index = 0; index < 5; index++)
    {
        Fixture(original, 19);
        original[unknown_fields[index]] ^= 1;
        FixCRC(original);
        memset(candidate, 0xA5, sizeof(candidate));
        D5NextPinkAudit audit;
        CHECK(D5NextPinkTest::BuildPlan(original, 809, candidate, 809, audit) != D5NEXT_OK);
        for(unsigned int offset = 0; offset < 809; offset++)
        {
            CHECK(candidate[offset] == 0xA5);
        }
    }
    Fixture(original, 19);
    D5NextPinkAudit audit;
    CHECK(D5NextPinkTest::BuildPlan(original, 808, candidate, 809, audit) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextPinkTest::BuildPlan(original, 809, candidate, 808, audit) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextPinkTest::BuildPlan(nullptr, 809, candidate, 809, audit) == D5NEXT_INVALID_ARGUMENT);
    CHECK(D5NextPinkTest::BuildPlan(original, 809, nullptr, 809, audit) == D5NEXT_INVALID_ARGUMENT);
    std::puts("PASS exact pink plan, all opaque bytes, stricter mask, changed baseline, and failure output");
}

static unsigned int opens;
static unsigned int closes;
static unsigned int reads;
static unsigned int gets;
static unsigned int sets;
static unsigned int raw_queries;
static bool locked;
static bool flushed;
static bool flush_failed;
static bool permitted;
static bool late_write_denied;
static bool lock_failed;
static bool identity_changed;
static int poll_result;
static int input_result;
static int get_result;
static int set_result;
static unsigned char input_id;
static unsigned int firmware;
static unsigned char configuration[809];

extern "C" int __wrap_stat(const char* path, struct stat* value)
{
    CHECK(strcmp(path, "/offline/pink") == 0);
    memset(value, 0, sizeof(*value));
    value->st_mode = S_IFCHR | 0600;
    value->st_ino = 123;
    value->st_rdev = 456;
    return 0;
}

extern "C" int __wrap_fstat(int fd, struct stat* value)
{
    CHECK(fd == 4242);
    __wrap_stat("/offline/pink", value);
    if(identity_changed)
    {
        value->st_ino++;
    }
    return 0;
}

extern "C" int __wrap_faccessat(int directory, const char* path, int mode, int flags)
{
    CHECK(directory == AT_FDCWD && flags == AT_EACCESS && strcmp(path, "/offline/pink") == 0);
    bool allowed = permitted && !(mode == W_OK && late_write_denied);
    errno = allowed ? 0 : EACCES;
    return allowed ? 0 : -1;
}

extern "C" int __wrap_open(const char* path, int flags, ...)
{
    CHECK(strcmp(path, "/offline/pink") == 0);
    CHECK((flags & (O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)) == (O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
    CHECK((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR);
    opens++;
    return 4242;
}

extern "C" int __wrap___open_2(const char* path, int flags)
{
    return __wrap_open(path, flags);
}

extern "C" int __wrap_close(int fd)
{
    CHECK(fd == 4242);
    closes++;
    locked = false;
    return 0;
}

extern "C" int __wrap_flock(int fd, int operation)
{
    CHECK(fd == 4242 && operation == (LOCK_EX | LOCK_NB));
    errno = lock_failed ? EWOULDBLOCK : 0;
    locked = !lock_failed;
    return lock_failed ? -1 : 0;
}

extern "C" int __wrap_fflush(FILE* stream)
{
    CHECK(stream == stdout);
    flushed = !flush_failed;
    return flush_failed ? EOF : 0;
}

extern "C" int __wrap_poll(struct pollfd* fds, nfds_t count, int timeout)
{
    CHECK(count == 1 && fds[0].fd == 4242 && fds[0].events == POLLIN && timeout == 2000 && locked);
    fds[0].revents = poll_result == 1 ? POLLIN : 0;
    return poll_result;
}

extern "C" ssize_t __wrap_read(int fd, void* data, std::size_t length)
{
    CHECK(fd == 4242 && length == 163 && locked);
    reads++;
    unsigned char* status = (unsigned char*)data;
    memset(status, 0, length);
    status[0] = input_id;
    status[13] = (unsigned char)(firmware >> 8);
    status[14] = (unsigned char)firmware;
    return input_result;
}

extern "C" int __wrap___poll_chk(struct pollfd* fds, nfds_t count, int timeout, std::size_t capacity)
{
    CHECK(capacity >= count * sizeof(struct pollfd));
    return __wrap_poll(fds, count, timeout);
}

extern "C" ssize_t __wrap___read_chk(int fd, void* data, std::size_t length, std::size_t capacity)
{
    CHECK(length <= capacity);
    return __wrap_read(fd, data, length);
}

extern "C" int __wrap_ioctl(int fd, unsigned long request, ...)
{
    CHECK(fd == 4242 && opens == 1 && closes == 0 && locked);
    va_list args;
    va_start(args, request);
    void* data = va_arg(args, void*);
    va_end(args);
    if(request == HIDIOCGRAWINFO)
    {
        raw_queries++;
        struct hidraw_devinfo* identity = (struct hidraw_devinfo*)data;
        identity->bustype = BUS_USB;
        identity->vendor = 0x0C70;
        identity->product = (short)0xF00E;
        return 0;
    }
    if(request == HIDIOCGFEATURE(809))
    {
        gets++;
        CHECK(reads == 1 && gets == 1 && sets == 0);
        CHECK(((unsigned char*)data)[0] == 3);
        memcpy(data, configuration, 809);
        errno = get_result < 0 ? EPIPE : 0;
        return get_result;
    }
    CHECK(request == HIDIOCSFEATURE(809));
    CHECK(gets == 1 && sets == 0 && flushed);
    D5NextPinkAudit audit;
    CHECK(D5NextPinkTest::AuditPlan(configuration, 809, (unsigned char*)data, 809, audit) == D5NEXT_OK);
    sets++;
    errno = set_result < 0 ? EPIPE : 0;
    return set_result;
}

static void Reset()
{
    opens = closes = reads = gets = sets = raw_queries = 0;
    locked = false;
    flushed = flush_failed = false;
    permitted = true;
    late_write_denied = lock_failed = identity_changed = false;
    poll_result = 1;
    input_result = 162;
    input_id = 1;
    firmware = 0xFA01;
    get_result = set_result = 809;
    Fixture(configuration, 77);
}

static void TestTransactions()
{
    D5NextDeviceInfo device;
    device.dev_node = "/offline/pink";
    D5NextDescriptor descriptor;
    descriptor.feature_bits[3] = 6464;
    descriptor.input_bits[1] = 1288;
    for(unsigned int scenario = 0; scenario < 19; scenario++)
    {
        Reset();
        bool competing = false;
        bool write_once = scenario != 0;
        bool recovery = true;
        bool inventory_complete = true;
        switch(scenario)
        {
        case 1: permitted = false; break;
        case 2: competing = true; break;
        case 3: lock_failed = true; break;
        case 4: identity_changed = true; break;
        case 5: poll_result = 0; break;
        case 6: input_result = 161; break;
        case 7: input_id = 2; break;
        case 8: get_result = -1; break;
        case 9: get_result = 808; break;
        case 10: configuration[10] ^= 1; break;
        case 11: firmware = 1; break;
        case 12: recovery = false; break;
        case 13: late_write_denied = true; break;
        case 14: set_result = -1; break;
        case 15: set_result = 808; break;
        case 17: inventory_complete = false; break;
        case 18: flush_failed = true; break;
        default: break;
        }
        bool approved = D5NextPinkTest::FirmwareIsApproved(firmware);
        D5NextPinkTest transaction;
        int result = transaction.Run(device, descriptor, competing, write_once, recovery, inventory_complete);
        bool should_set = scenario >= 14 && scenario <= 16 && approved;
        CHECK(sets == (should_set ? 1U : 0U));
        CHECK(result == (scenario == 0 || (scenario == 16 && approved) ? 0 : (should_set ? 5 : 4)));
        /* Scenario 16 is the successful test-only synthetic firmware path. */
        if(scenario == 16 && approved)
        {
            CHECK(result == 0 && sets == 1);
        }
        CHECK(opens == closes);
        CHECK(gets <= 1 && reads <= 1 && sets <= 1);
        unsigned int old_gets = gets;
        CHECK(transaction.Run(device, descriptor, false, write_once, recovery, inventory_complete) == 4);
        CHECK(gets == old_gets);
    }
    Reset();
    D5NextPinkTest serialized;
    int first_result;
    int second_result;
    std::thread first([&]()
    {
        first_result = serialized.Run(device, descriptor, false, true, true, true);
    });
    std::thread second([&]()
    {
        second_result = serialized.Run(device, descriptor, false, true, true, true);
    });
    first.join();
    second.join();
    CHECK(opens == 1 && closes == 1 && gets == 1 && reads == 1);
    if(D5NextPinkTest::FirmwareIsApproved(0xFA01))
    {
        CHECK(sets == 1 && ((first_result == 0 && second_result == 4) || (first_result == 4 && second_result == 0)));
    }
    else
    {
        CHECK(sets == 0 && first_result == 4 && second_result == 4);
    }
    std::puts("PASS one locked handle, passive firmware, fresh GET, gates, and no retry after any result");
}

int main()
{
    TestPlans();
#ifndef OPENRGB_D5NEXT_PINK_OFFLINE_TESTS
    for(unsigned int value = 0; value <= 65535; value++)
    {
        CHECK(!D5NextPinkTest::FirmwareIsApproved(value));
    }
    std::puts("PASS production refuses every 16-bit firmware value (including the synthetic identity)");
#endif
    TestTransactions();
    std::printf("PASS pink preparation tests (%u checks); device syscalls are mocked\n", checks);
    return 0;
}
