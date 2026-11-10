/*---------------------------------------------------------*\
| D5NextDiagnostic.cpp                                      |
|                                                           |
|   Linux metadata and one-shot read-only HID diagnostic    |
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
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include "D5NextReadOnly.h"

static void PrintText(const char* label, const std::filesystem::path& path)
{
    std::string value;
    if(D5NextReadOnly::ReadText(path, value))
    {
        std::printf("%s: %s\n", label, value.c_str());
    }
    else
    {
        std::printf("%s: unavailable\n", label);
    }
}

static void PrintTelemetry(const D5NextDeviceInfo& device)
{
    std::error_code error;
    std::filesystem::directory_iterator entries(device.hid_path / "hwmon", error);
    if(error)
    {
        std::puts("hwmon: unavailable");
        return;
    }
    /*-----------------------------------------------------*\
    | Read only cached *_input and labels. In the installed |
    | driver, reading offsets/PWM can initiate config GETs.  |
    | Never read those controls, even through sysfs.         |
    \*-----------------------------------------------------*/
    const std::regex input_name("(temp[0-9]+|fan[0-9]+|power[0-9]+|in[0-9]+|curr[0-9]+)_input");
    for(const std::filesystem::directory_entry& entry : entries)
    {
        std::string name;
        if(!D5NextReadOnly::ReadText(entry.path() / "name", name) || name != "d5next")
        {
            continue;
        }
        std::printf("hwmon: %s (one snapshot, raw Linux hwmon units)\n", name.c_str());
        std::filesystem::directory_iterator sensors(entry.path(), error);
        if(error)
        {
            continue;
        }
        for(const std::filesystem::directory_entry& sensor : sensors)
        {
            std::string filename = sensor.path().filename().string();
            if(!std::regex_match(filename, input_name))
            {
                continue;
            }
            std::string label;
            std::string value;
            D5NextReadOnly::ReadText(entry.path() / (filename.substr(0, filename.size() - 6) + "_label"), label);
            bool available = D5NextReadOnly::ReadText(sensor.path(), value);
            std::printf("  %s [%s]: %s\n", filename.c_str(), label.c_str(), available ? value.c_str() : "unavailable");
        }
    }
}

static int RunDiagnostic(int argc, char** argv)
{
    bool get_feature = argc == 2 && strcmp(argv[1], "--get-feature") == 0;
    if(argc > 2 || (argc == 2 && !get_feature))
    {
        std::fprintf(stderr, "Usage: d5next-readonly [--get-feature]\nDefault: cached metadata only. No manual device path or write command.\n");
        return 2;
    }
    std::puts("D5 NEXT read-only diagnostic; no raw configuration or serial numbers are printed or saved.");
    std::vector<D5NextDeviceInfo> devices = D5NextReadOnly::Discover();
    std::printf("USB 0C70:F00E, HID interface 1 matches: %zu\n", devices.size());
    if(devices.empty())
    {
        return 3;
    }
    std::vector<std::string> processes = D5NextReadOnly::FindControllerProcesses();
    for(unsigned int index = 0; index < processes.size(); index++)
    {
        std::printf("possible competing process: %s\n", processes[index].c_str());
    }
    int outcome = 0;
    for(unsigned int index = 0; index < devices.size(); index++)
    {
        const D5NextDeviceInfo& device = devices[index];
        std::printf("device: %s, interface=%s\n", device.dev_node.c_str(), device.interface_path.filename().c_str());
        PrintText("manufacturer", device.usb_path / "manufacturer");
        PrintText("product", device.usb_path / "product");
        PrintText("USB bcdDevice (not firmware approval)", device.usb_path / "bcdDevice");
        std::error_code error;
        std::filesystem::path driver = std::filesystem::read_symlink(device.hid_path / "driver", error);
        std::printf("HID driver: %s\n", error ? "unavailable" : driver.filename().c_str());
        struct stat permissions;
        if(stat(device.dev_node.c_str(), &permissions) == 0)
        {
            std::printf("permissions: mode=%03o uid=%u gid=%u effective_read=%s effective_write=%s\n",
                        permissions.st_mode & 0777, permissions.st_uid, permissions.st_gid,
                        faccessat(AT_FDCWD, device.dev_node.c_str(), R_OK, AT_EACCESS) == 0 ? "yes" : "no",
                        faccessat(AT_FDCWD, device.dev_node.c_str(), W_OK, AT_EACCESS) == 0 ? "yes" : "no");
        }
        std::vector<unsigned char> bytes;
        D5NextDescriptor descriptor;
        bool parsed = D5NextReadOnly::ReadDescriptor(device.hid_path / "report_descriptor", bytes)
                   && D5NextReadOnly::ParseDescriptor(bytes, descriptor);
        std::printf("cached HID descriptor: %zu bytes, parsed=%s\n", bytes.size(), parsed ? "yes" : "no");
        if(parsed)
        {
            for(unsigned int id = 0; id < 256; id++)
            {
                if(descriptor.input_bits[id] || descriptor.output_bits[id] || descriptor.feature_bits[id])
                {
                    std::printf("  report=0x%02X input_bits=%u output_bits=%u feature_bits=%u\n", id,
                                descriptor.input_bits[id], descriptor.output_bits[id], descriptor.feature_bits[id]);
                }
            }
        }
        PrintTelemetry(device);
        std::filesystem::path firmware = std::filesystem::path("/sys/kernel/debug")
                                      / ("aquacomputer_d5next-" + device.hid_path.filename().string()) / "firmware_version";
        PrintText("cached firmware version (debugfs)", firmware);
        std::puts("layout compatibility: UNCONFIRMED; all writes remain forbidden");
        if(!get_feature)
        {
            std::puts("GET_FEATURE: not requested; device not opened");
            continue;
        }
        if(devices.size() != 1 || !parsed)
        {
            std::puts("GET_FEATURE: not attempted (ambiguous identity or invalid descriptor)");
            outcome = 4;
            continue;
        }
        unsigned char report[D5NEXT_REPORT_SIZE];
        int received;
        int system_error;
        D5NextResult validation;
        D5NextReadResult result = D5NextReadOnly::GetFeatureOnce(device, descriptor, !processes.empty(), report,
                                                                received, system_error, validation);
        std::printf("GET_FEATURE: %s; received=%d; validation=%s\n", D5NextReadOnly::ReadResultString(result),
                    received, D5NextProtocol::ResultString(validation));
        if(system_error != 0)
        {
            std::printf("system error: %s\n", strerror(system_error));
        }
        if(result != D5NEXT_READ_OK)
        {
            outcome = 4;
            continue;
        }
        std::printf("report: id=0x%02X length=%d stored_crc=0x%02X%02X calculated_crc=0x%04X\n",
                    report[0], received, report[807], report[808], D5NextProtocol::CalculateCRC(report + 1, 806));
        std::printf("candidate RGB fields (layout assumption only): brightness=%u flag=0x%02X effect=0x%02X hue=%u saturation=%u\n",
                    report[237], report[239], report[663], (report[706] << 8) | report[707], report[708]);
        D5NextLighting lighting;
        D5NextResult decoded = D5NextProtocol::Decode(report, received, D5NEXT_LAYOUT_DOCUMENTED_809, lighting);
        std::printf("strict codec field decode: %s (not firmware compatibility approval)\n", D5NextProtocol::ResultString(decoded));
    }
    return outcome;
}

int main(int argc, char** argv)
{
    try
    {
        return RunDiagnostic(argc, argv);
    }
    catch(const std::filesystem::filesystem_error& error)
    {
        std::fprintf(stderr, "Filesystem discovery stopped: %s; no retry\n", error.what());
        return 5;
    }
}
