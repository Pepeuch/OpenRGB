/*---------------------------------------------------------*\
| ReadOnlyTests.cpp                                         |
|                                                           |
|   Descriptor and read-only syscall boundary tests         |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
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
#include <fstream>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "D5NextReadOnly.h"

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

static bool permitted;
static bool open_fails;
static bool lock_fails;
static bool wrong_identity;
static bool changed_inode;
static int feature_result;
static unsigned int opens;
static unsigned int closes;
static unsigned int raw_queries;
static unsigned int feature_queries;
static unsigned char response[809];

extern "C" int __wrap_stat(const char* path, struct stat* status)
{
    CHECK(strcmp(path, "/offline/d5-next") == 0);
    memset(status, 0, sizeof(*status));
    status->st_mode = S_IFCHR | 0400;
    status->st_ino = 100;
    status->st_rdev = 123;
    return 0;
}

extern "C" int __wrap_fstat(int fd, struct stat* status)
{
    CHECK(fd == 4242);
    __wrap_stat("/offline/d5-next", status);
    if(changed_inode)
    {
        status->st_ino++;
    }
    return 0;
}

extern "C" int __wrap_faccessat(int directory, const char* path, int mode, int flags)
{
    CHECK(directory == AT_FDCWD && strcmp(path, "/offline/d5-next") == 0);
    CHECK(mode == R_OK && flags == AT_EACCESS);
    errno = permitted ? 0 : EACCES;
    return permitted ? 0 : -1;
}

extern "C" int __wrap_open(const char* path, int flags, ...)
{
    CHECK(strcmp(path, "/offline/d5-next") == 0);
    CHECK((flags & O_ACCMODE) == O_RDONLY);
    CHECK((flags & (O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)) == (O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW));
    opens++;
    errno = open_fails ? EACCES : 0;
    return open_fails ? -1 : 4242;
}

extern "C" int __wrap_close(int fd)
{
    CHECK(fd == 4242);
    closes++;
    return 0;
}

extern "C" int __wrap_flock(int fd, int operation)
{
    CHECK(fd == 4242 && operation == (LOCK_EX | LOCK_NB));
    errno = lock_fails ? EWOULDBLOCK : 0;
    return lock_fails ? -1 : 0;
}

extern "C" int __wrap_ioctl(int fd, unsigned long request, ...)
{
    CHECK(fd == 4242);
    va_list arguments;
    va_start(arguments, request);
    void* data = va_arg(arguments, void*);
    va_end(arguments);
    if(request == HIDIOCGRAWINFO)
    {
        raw_queries++;
        struct hidraw_devinfo* info = (struct hidraw_devinfo*)data;
        info->bustype = BUS_USB;
        info->vendor = 0x0C70;
        info->product = (short)(wrong_identity ? 0xF012 : 0xF00E);
        return 0;
    }
    CHECK(request == HIDIOCGFEATURE(809));
    feature_queries++;
    CHECK(((unsigned char*)data)[0] == 3);
    if(feature_result < 0)
    {
        errno = EPIPE;
        return -1;
    }
    memcpy(data, response, sizeof(response));
    return feature_result;
}

static void ResetMock()
{
    permitted = true;
    open_fails = lock_fails = wrong_identity = changed_inode = false;
    feature_result = 809;
    opens = closes = raw_queries = feature_queries = 0;
    memset(response, 0, sizeof(response));
    response[0] = 3;
    response[1] = 0;
    response[2] = 3;
    response[663] = 1;
    unsigned short crc = D5NextProtocol::CalculateCRC(response + 1, 806);
    response[807] = (unsigned char)(crc >> 8);
    response[808] = (unsigned char)crc;
}

static void TestDescriptors()
{
    D5NextDescriptor descriptor;
    std::vector<unsigned char> data = { 0x85, 3, 0x75, 8, 0x96, 0x28, 3, 0xB1, 2 };
    CHECK(D5NextReadOnly::ParseDescriptor(data, descriptor));
    CHECK(descriptor.feature_bits[3] == 6464 && descriptor.input_bits[3] == 0);
    CHECK(descriptor.feature_bits[2] == 0);
    data = { 0x85, 3, 0x75, 8, 0x95, 2, 0xB1, 2, 0xA4, 0x85, 2, 0x95, 10,
             0xB1, 2, 0xB4, 0x96, 0x26, 3, 0xB1, 2 };
    CHECK(D5NextReadOnly::ParseDescriptor(data, descriptor));
    CHECK(descriptor.feature_bits[3] == 6464 && descriptor.feature_bits[2] == 80);
    const std::vector<std::vector<unsigned char>> invalid =
    {
        {}, { 0x96, 0x28 }, { 0x85, 0 }, { 0xB4 }, { 0xA4 }, { 0xFE },
        { 0x85, 3, 0x75, 32, 0x97, 0xFF, 0xFF, 0xFF, 0xFF, 0xB1, 2 },
        { 0x75, 0, 0x95, 1, 0xB1, 2 },
    };
    for(unsigned int index = 0; index < invalid.size(); index++)
    {
        CHECK(!D5NextReadOnly::ParseDescriptor(invalid[index], descriptor));
    }
    std::puts("PASS read-only descriptor framing, counts, push/pop, and malformed items");
}

static void TestAcquisition()
{
    D5NextDeviceInfo device;
    device.dev_node = "/offline/d5-next";
    D5NextDescriptor descriptor;
    descriptor.feature_bits[3] = 6464;
    for(unsigned int scenario = 0; scenario < 11; scenario++)
    {
        ResetMock();
        unsigned char output[809];
        memset(output, 0xA5, sizeof(output));
        bool competing = false;
        switch(scenario)
        {
        case 1: permitted = false; break;
        case 2: competing = true; break;
        case 3: descriptor.feature_bits[3] = 6456; break;
        case 4: open_fails = true; break;
        case 5: lock_fails = true; break;
        case 6: wrong_identity = true; break;
        case 7: changed_inode = true; break;
        case 8: feature_result = -1; break;
        case 9: feature_result = 808; break;
        case 10: response[10] ^= 1; break;
        }
        int received;
        int system_error;
        D5NextResult validation;
        D5NextReadResult result = D5NextReadOnly::GetFeatureOnce(device, descriptor, competing,
                                                                output, received, system_error, validation);
        descriptor.feature_bits[3] = 6464;
        const D5NextReadResult expected[] =
        {
            D5NEXT_READ_OK, D5NEXT_READ_PERMISSION_DENIED, D5NEXT_READ_COMPETING_PROCESS,
            D5NEXT_READ_DESCRIPTOR_MISMATCH, D5NEXT_READ_OPEN_FAILED, D5NEXT_READ_LOCK_FAILED,
            D5NEXT_READ_IDENTITY_FAILED, D5NEXT_READ_IDENTITY_FAILED, D5NEXT_READ_TRANSFER_FAILED,
            D5NEXT_READ_INVALID_REPORT, D5NEXT_READ_INVALID_REPORT
        };
        CHECK(result == expected[scenario]);
        CHECK(opens == (scenario >= 1 && scenario <= 3 ? 0U : 1U));
        CHECK(closes == (scenario >= 1 && scenario <= 4 ? 0U : 1U));
        CHECK(feature_queries == (scenario == 0 || scenario >= 8 ? 1U : 0U));
        if(scenario == 0)
        {
            CHECK(received == 809 && system_error == 0 && validation == D5NEXT_OK);
            CHECK(memcmp(output, response, sizeof(output)) == 0);
        }
        else
        {
            for(unsigned int offset = 0; offset < 809; offset++)
            {
                CHECK(output[offset] == 0xA5);
            }
        }
        if(scenario == 8)
        {
            CHECK(received == -1 && system_error == EPIPE && feature_queries == 1);
        }
    }
    std::puts("PASS permissions-before-open, O_RDONLY, identity, one GET, errors, and handle cleanup");
}

static void WriteText(const std::filesystem::path& path, const char* text)
{
    std::ofstream stream(path);
    stream << text;
    CHECK(stream.good());
}

static void TestDiscovery()
{
    std::filesystem::path root = std::filesystem::current_path() / ".build/d5next-readonly/fixture";
    std::filesystem::remove_all(root);
    std::filesystem::path usb = root / "usb";
    std::filesystem::path interface = usb / "usb:1.1";
    std::filesystem::path hid = interface / "hid-device";
    std::filesystem::create_directories(hid);
    std::filesystem::create_directories(root / "class/hidraw42");
    std::filesystem::create_symlink(hid, root / "class/hidraw42/device");
    WriteText(usb / "idVendor", "0c70\n");
    WriteText(usb / "idProduct", "f00e\n");
    WriteText(interface / "bInterfaceNumber", "01\n");
    WriteText(interface / "bInterfaceClass", "03\n");
    std::vector<D5NextDeviceInfo> devices = D5NextReadOnly::Discover(root / "class", root / "dev");
    CHECK(devices.size() == 1);
    CHECK(devices[0].dev_node == root / "dev/hidraw42");
    WriteText(interface / "bInterfaceNumber", "00\n");
    CHECK(D5NextReadOnly::Discover(root / "class", root / "dev").empty());
    WriteText(interface / "bInterfaceNumber", "01\n");
    WriteText(usb / "idProduct", "f012\n");
    CHECK(D5NextReadOnly::Discover(root / "class", root / "dev").empty());
    CHECK(D5NextReadOnly::Discover(root / "absent", root / "dev").empty());
    std::filesystem::create_directories(root / "proc/123");
    WriteText(root / "proc/123/comm", "OpenRGB\n");
    CHECK(D5NextReadOnly::FindControllerProcesses(root / "proc").size() == 1);
    std::filesystem::remove_all(root);
    std::puts("PASS automatic sysfs identity/interface discovery and competing-process detection");
}

int main()
{
    TestDescriptors();
    TestAcquisition();
    TestDiscovery();
    std::printf("PASS read-only diagnostic offline tests (%u checks); HID syscalls wrapped\n", checks);
    return 0;
}
