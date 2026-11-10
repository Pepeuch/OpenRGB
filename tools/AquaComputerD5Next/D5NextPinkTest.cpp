/*---------------------------------------------------------*\
| D5NextPinkTest.cpp                                        |
|                                                           |
|   Audited one-shot D5 NEXT pink test preparation           |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "D5NextPinkTest.h"

#define D5NEXT_STATUS_SIZE             162
#define D5NEXT_STATUS_FIRMWARE_OFFSET  13

static bool IsFirstTestBaseline(const D5NextLighting& lighting)
{
    return lighting.brightness == 255 && lighting.enabled == D5NEXT_LIGHTING_ON
        && lighting.effect == D5NEXT_EFFECT_RAINBOW && lighting.hue == 0 && lighting.saturation == 255;
}

static bool IsPinkChangeOffset(unsigned int offset)
{
    /*-----------------------------------------------------*\
    | For this exact initial state, even brightness/enable  |
    | must remain unchanged. Narrower than the general mask.|
    \*-----------------------------------------------------*/
    return offset == D5NEXT_EFFECT_OFFSET || offset == D5NEXT_HUE_OFFSET
        || offset == D5NEXT_HUE_OFFSET + 1 || offset == D5NEXT_SATURATION_OFFSET
        || offset == D5NEXT_CRC_OFFSET || offset == D5NEXT_CRC_OFFSET + 1;
}

D5NextResult D5NextPinkTest::AuditPlan(const unsigned char* original, std::size_t size,
                                     const unsigned char* candidate, std::size_t candidate_size,
                                     D5NextPinkAudit& audit)
{
    D5NextPinkAudit checked = {};
    D5NextResult result = D5NextProtocol::Decode(original, size, D5NEXT_LAYOUT_DOCUMENTED_809, checked.initial);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(!IsFirstTestBaseline(checked.initial))
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    result = D5NextProtocol::Decode(candidate, candidate_size, D5NEXT_LAYOUT_DOCUMENTED_809, checked.target);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(checked.target.effect != D5NEXT_EFFECT_STATIC || checked.target.enabled != D5NEXT_LIGHTING_ON
    || checked.target.brightness != 255 || checked.target.hue != 1408 || checked.target.saturation != 150)
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    result = D5NextProtocol::ValidateLightingChange(original, size, candidate, candidate_size,
                                                   D5NEXT_LAYOUT_DOCUMENTED_809);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    checked.initial_crc = (unsigned short)((original[807] << 8) | original[808]);
    checked.calculated_initial_crc = D5NextProtocol::CalculateCRC(original + 1, 806);
    checked.target_crc = D5NextProtocol::CalculateCRC(candidate + 1, 806);
    for(unsigned int offset = 0; offset < D5NEXT_REPORT_SIZE; offset++)
    {
        if(original[offset] != candidate[offset])
        {
            if(!IsPinkChangeOffset(offset) || checked.changed_count >= 8)
            {
                return D5NEXT_UNEXPECTED_CHANGE;
            }
            checked.offsets[checked.changed_count] = offset;
            checked.before[checked.changed_count] = original[offset];
            checked.after[checked.changed_count] = candidate[offset];
            checked.changed_count++;
        }
    }
    if(checked.changed_count != 6)
    {
        return D5NEXT_UNEXPECTED_CHANGE;
    }
    audit = checked;
    return D5NEXT_OK;
}

D5NextResult D5NextPinkTest::BuildPlan(const unsigned char* original, std::size_t size,
                                     unsigned char* candidate, std::size_t candidate_size,
                                     D5NextPinkAudit& audit)
{
    if(candidate == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(candidate_size != D5NEXT_REPORT_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    D5NextLighting initial;
    D5NextResult result = D5NextProtocol::Decode(original, size, D5NEXT_LAYOUT_DOCUMENTED_809, initial);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(!IsFirstTestBaseline(initial))
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    D5NextLighting pink = D5NextProtocol::RGBToHSV(255, 105, 180);
    unsigned char prepared[D5NEXT_REPORT_SIZE];
    result = D5NextProtocol::BuildLightingReport(original, size, D5NEXT_LAYOUT_DOCUMENTED_809, pink,
                                                prepared, sizeof(prepared));
    if(result != D5NEXT_OK)
    {
        return result;
    }
    result = AuditPlan(original, size, prepared, sizeof(prepared), audit);
    if(result == D5NEXT_OK)
    {
        memcpy(candidate, prepared, sizeof(prepared));
    }
    return result;
}

bool D5NextPinkTest::FirmwareIsApproved(unsigned int firmware)
{
#ifdef OPENRGB_D5NEXT_PINK_OFFLINE_TESTS
    /* Synthetic test identity only; never enabled by the production build. */
    return firmware == 0xFA01;
#else
    /* No demonstrated firmware/layout mapping has been approved for writes. */
    (void)firmware;
    return false;
#endif
}

void D5NextPinkTest::PrintAudit(const D5NextPinkAudit& audit)
{
    std::printf("initial: brightness=%u flag=0x%02X effect=0x%02X hue=%u saturation=%u\n",
                audit.initial.brightness, audit.initial.enabled, audit.initial.effect,
                audit.initial.hue, audit.initial.saturation);
    std::printf("target: #FF69B4, brightness=255 (100%%), flag=0x00, static=0x01, hue=%u (0x%04X), saturation=%u\n",
                audit.target.hue, audit.target.hue, audit.target.saturation);
    std::printf("CRC: initial_stored=0x%04X initial_calculated=0x%04X candidate=0x%04X\n",
                audit.initial_crc, audit.calculated_initial_crc, audit.target_crc);
    std::printf("byte audit: %u changed bytes; offsets include report ID\n", audit.changed_count);
    for(unsigned int index = 0; index < audit.changed_count; index++)
    {
        std::printf("  offset=%u (0x%03X): 0x%02X -> 0x%02X\n", audit.offsets[index], audit.offsets[index],
                    audit.before[index], audit.after[index]);
    }
    std::puts("all other bytes: unchanged (including all pump/fan/sensor/curve bytes)");
    std::puts("six integrated LEDs uniformly pink: expected result, not yet demonstrated on hardware");
}

D5NextPinkTest::D5NextPinkTest()
{
    fd = -1;
    consumed = false;
}

D5NextPinkTest::~D5NextPinkTest()
{
    if(fd >= 0)
    {
        close(fd);
    }
}

int D5NextPinkTest::Finish(int result)
{
    if(fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    return result;
}

int D5NextPinkTest::Run(const D5NextDeviceInfo& device, const D5NextDescriptor& descriptor,
                       bool competing_process, bool write_once, bool recovery_confirmed,
                       bool process_inventory_complete)
{
    std::lock_guard<std::mutex> lock(transaction_mutex);
    if(consumed)
    {
        std::puts("BLOCKED: this transaction was already attempted; no retry");
        return 4;
    }
    consumed = true;
    if(competing_process)
    {
        std::puts("BLOCKED: possible competing controller/handle; not opened");
        return 4;
    }
    if(descriptor.feature_bits[3] != 808 * 8 || descriptor.input_bits[1] != 161 * 8)
    {
        std::puts("BLOCKED: status/config descriptor framing is not confirmed; not opened");
        return 4;
    }
    struct stat before;
    int required_access = write_once ? R_OK | W_OK : R_OK;
    if(stat(device.dev_node.c_str(), &before) != 0 || !S_ISCHR(before.st_mode)
    || faccessat(AT_FDCWD, device.dev_node.c_str(), required_access, AT_EACCESS) != 0)
    {
        std::puts("BLOCKED: required HID permissions unavailable; not opened; no permission change");
        return 4;
    }
    fd = open(device.dev_node.c_str(), (write_once ? O_RDWR : O_RDONLY) | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if(fd < 0)
    {
        std::printf("BLOCKED: opening failed: %s; no retry\n", strerror(errno));
        return 4;
    }
    if(flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        std::puts("BLOCKED: cooperative lock unavailable; no request");
        return Finish(4);
    }
    struct stat after;
    struct hidraw_devinfo identity = {};
    if(fstat(fd, &after) != 0 || !S_ISCHR(after.st_mode)
    || before.st_ino != after.st_ino || before.st_rdev != after.st_rdev
    || ioctl(fd, HIDIOCGRAWINFO, &identity) < 0 || identity.bustype != BUS_USB
    || identity.vendor != D5NEXT_VID || (unsigned short)identity.product != D5NEXT_PID)
    {
        std::puts("BLOCKED: device identity not confirmed; no feature request");
        return Finish(4);
    }

    /*-----------------------------------------------------*\
    | One passive periodic status report, not a control GET.|
    | Firmware offset is documented by the installed kernel |
    | driver. No raw status/serial bytes are logged.         |
    \*-----------------------------------------------------*/
    struct pollfd readiness = { fd, POLLIN, 0 };
    int polled = poll(&readiness, 1, 2000);
    if(polled != 1 || (readiness.revents & (POLLERR | POLLHUP | POLLNVAL))
    || !(readiness.revents & POLLIN))
    {
        std::puts("BLOCKED: passive firmware status unavailable; no retry or fallback GET");
        return Finish(4);
    }
    unsigned char status[D5NEXT_STATUS_SIZE + 1];
    ssize_t status_size = read(fd, status, sizeof(status));
    if(status_size != D5NEXT_STATUS_SIZE || status[0] != 0x01)
    {
        std::puts("BLOCKED: unexpected passive status framing; no retry");
        return Finish(4);
    }
    unsigned int firmware = (status[D5NEXT_STATUS_FIRMWARE_OFFSET] << 8)
                          | status[D5NEXT_STATUS_FIRMWARE_OFFSET + 1];
    bool approved = FirmwareIsApproved(firmware);
    std::printf("firmware from passive Input 0x01: %u (0x%04X); layout write approval=%s\n",
                firmware, firmware, approved ? "yes" : "NO");
    std::printf("recovery via aquasuite: %s\n", recovery_confirmed ? "operator confirmed available" : "not confirmed");

    unsigned char original[D5NEXT_REPORT_SIZE] = { D5NEXT_REPORT_ID };
    /* One fresh GET under the same lock/handle as the optional single SET. */
    int received = ioctl(fd, HIDIOCGFEATURE(sizeof(original)), original);
    if(received < 0)
    {
        std::printf("BLOCKED: GET_FEATURE failed: %s; no retry\n", strerror(errno));
        return Finish(4);
    }
    D5NextResult validation = D5NextProtocol::ValidateReport(original, (std::size_t)received);
    std::printf("fresh configuration: received=%d id=0x%02X validation=%s\n",
                received, original[0], D5NextProtocol::ResultString(validation));
    if(validation != D5NEXT_OK)
    {
        return Finish(4);
    }
    unsigned char candidate[D5NEXT_REPORT_SIZE];
    D5NextPinkAudit audit;
    validation = BuildPlan(original, sizeof(original), candidate, sizeof(candidate), audit);
    if(validation != D5NEXT_OK)
    {
        std::printf("BLOCKED: candidate audit failed: %s (expected the confirmed full-brightness Rainbow baseline)\n",
                    D5NextProtocol::ResultString(validation));
        return Finish(4);
    }
    PrintAudit(audit);
    if(!approved)
    {
        std::puts("WRITE BLOCKER: firmware/layout mapping has not been approved; no override available");
    }
    if(!recovery_confirmed)
    {
        std::puts("WRITE BLOCKER: aquasuite recovery access must be confirmed");
    }
    if(!process_inventory_complete)
    {
        std::puts("WRITE BLOCKER: device-handle inventory is incomplete; hidden clients cannot be excluded");
    }
    bool writable_now = faccessat(AT_FDCWD, device.dev_node.c_str(), W_OK, AT_EACCESS) == 0;
    if(!writable_now)
    {
        std::puts("WRITE BLOCKER: node does not permit O_RDWR; permissions are not changed");
    }
    std::puts("limits: lock is cooperative; no proven exclusion of hidden/non-cooperating clients");
    std::puts("limits: report 0x03 persistence and RGB group coverage remain unverified");
    if(!write_once)
    {
        std::puts("SIMULATION COMPLETE: no SET/Output/save/reset/restore was sent");
        return Finish(0);
    }
    if(!approved || !recovery_confirmed || !writable_now || !process_inventory_complete)
    {
        return Finish(4);
    }
    /*-----------------------------------------------------*\
    | Re-audit immediately before the sole possible SET.    |
    | Production has an empty approval policy, so cannot    |
    | reach this branch. No retry, readback, save or restore.|
    \*-----------------------------------------------------*/
    if(AuditPlan(original, sizeof(original), candidate, sizeof(candidate), audit) != D5NEXT_OK)
    {
        return Finish(4);
    }
    if(fflush(stdout) != 0)
    {
        std::fprintf(stderr, "STOP: audit output could not be flushed; no SET sent\n");
        return Finish(4);
    }
    int sent = ioctl(fd, HIDIOCSFEATURE(sizeof(candidate)), candidate);
    if(sent != D5NEXT_REPORT_SIZE)
    {
        std::puts("STOP: SET completion uncertain; no retry or automatic recovery");
        return Finish(5);
    }
    std::puts("single SET completed; inspect LEDs/pump manually; no save or further HID commands");
    return Finish(0);
}
