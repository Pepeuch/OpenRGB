/*---------------------------------------------------------*\
| D5NextPinkMain.cpp                                        |
|                                                           |
|   Dedicated pink test, read-only simulation by default     |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "D5NextPinkTest.h"

static bool FindVisibleHandleOwners(const D5NextDeviceInfo& device, bool& inventory_complete)
{
    inventory_complete = false;
    std::string bus;
    std::string number;
    if(!D5NextReadOnly::ReadText(device.usb_path / "busnum", bus)
    || !D5NextReadOnly::ReadText(device.usb_path / "devnum", number))
    {
        std::puts("BLOCKED: USB node identity unavailable for competing-handle inventory");
        return true;
    }
    char usb_node[64];
    snprintf(usb_node, sizeof(usb_node), "/dev/bus/usb/%03u/%03u",
             (unsigned int)strtoul(bus.c_str(), nullptr, 10), (unsigned int)strtoul(number.c_str(), nullptr, 10));
    unsigned int inaccessible = 0;
    unsigned int holders = 0;
    std::error_code error;
    std::filesystem::directory_iterator processes("/proc", error);
    if(error)
    {
        std::puts("BLOCKED: process/FD inventory unavailable");
        return true;
    }
    for(const std::filesystem::directory_entry& process : processes)
    {
        std::string pid = process.path().filename().string();
        if(pid.empty() || pid.find_first_not_of("0123456789") != std::string::npos)
        {
            continue;
        }
        std::filesystem::directory_iterator handles(process.path() / "fd", error);
        if(error)
        {
            inaccessible++;
            continue;
        }
        for(const std::filesystem::directory_entry& handle : handles)
        {
            std::filesystem::path target = std::filesystem::read_symlink(handle.path(), error);
            if(error == std::errc::permission_denied)
            {
                inaccessible++;
            }
            if(!error && (target == device.dev_node || target == usb_node))
            {
                std::printf("possible competing handle: pid=%s node=%s\n", pid.c_str(), target.c_str());
                holders++;
            }
        }
    }
    std::printf("visible device holders=%u; inaccessible process FD paths=%u (exclusive access not proven)\n",
                holders, inaccessible);
    inventory_complete = inaccessible == 0;
    return holders != 0;
}

static void PrintPumpSnapshot(const D5NextDeviceInfo& device)
{
    std::error_code error;
    std::filesystem::directory_iterator entries(device.hid_path / "hwmon", error);
    if(error)
    {
        std::puts("pump monitoring: cached hwmon unavailable");
        return;
    }
    for(const std::filesystem::directory_entry& entry : entries)
    {
        std::string name;
        if(!D5NextReadOnly::ReadText(entry.path() / "name", name) || name != "d5next")
        {
            continue;
        }
        std::string speed;
        std::string temperature;
        bool speed_ok = D5NextReadOnly::ReadText(entry.path() / "fan1_input", speed);
        bool temperature_ok = D5NextReadOnly::ReadText(entry.path() / "temp1_input", temperature);
        std::printf("cached pump snapshot: rpm=%s coolant_millidegrees_C=%s\n",
                    speed_ok ? speed.c_str() : "unavailable", temperature_ok ? temperature.c_str() : "unavailable");
    }
}

static int RunPinkTest(int argc, char** argv)
{
    bool write_once = false;
    bool recovery_confirmed = false;
    for(int argument = 1; argument < argc; argument++)
    {
        if(strcmp(argv[argument], "--write-once") == 0 && !write_once)
        {
            write_once = true;
        }
        else if(strcmp(argv[argument], "--recovery-aquasuite-confirmed") == 0 && !recovery_confirmed)
        {
            recovery_confirmed = true;
        }
        else
        {
            std::fprintf(stderr, "Usage: d5next-pink-test [--write-once] [--recovery-aquasuite-confirmed]\n"
                                 "Default is a read-only simulation. No firmware override or manual USB path.\n");
            return 2;
        }
    }
    std::printf("mode: %s\n", write_once ? "SINGLE WRITE REQUESTED (requires all safety gates)" : "READ-ONLY SIMULATION");
    std::puts("No raw config/status dump, save, Output, reset, retry or automatic restore is implemented.");
    std::vector<D5NextDeviceInfo> devices = D5NextReadOnly::Discover();
    if(devices.size() != 1)
    {
        std::printf("BLOCKED: expected exactly one D5 NEXT HID interface 1, found %zu\n", devices.size());
        return 4;
    }
    const D5NextDeviceInfo& device = devices[0];
    std::printf("automatically discovered: 0C70:F00E interface=1 node=%s\n", device.dev_node.c_str());
    std::vector<std::string> controllers = D5NextReadOnly::FindControllerProcesses();
    for(unsigned int index = 0; index < controllers.size(); index++)
    {
        std::printf("possible competing controller: %s\n", controllers[index].c_str());
    }
    bool inventory_complete;
    bool held = FindVisibleHandleOwners(device, inventory_complete);
    std::vector<unsigned char> bytes;
    D5NextDescriptor descriptor;
    if(!D5NextReadOnly::ReadDescriptor(device.hid_path / "report_descriptor", bytes)
    || !D5NextReadOnly::ParseDescriptor(bytes, descriptor))
    {
        std::puts("BLOCKED: cached HID descriptor unavailable or malformed");
        return 4;
    }
    PrintPumpSnapshot(device);
    D5NextPinkTest transaction;
    return transaction.Run(device, descriptor, !controllers.empty() || held, write_once, recovery_confirmed,
                            inventory_complete);
}

int main(int argc, char** argv)
{
    try
    {
        return RunPinkTest(argc, argv);
    }
    catch(const std::filesystem::filesystem_error& error)
    {
        std::fprintf(stderr, "STOP: filesystem inventory failed: %s; no retry\n", error.what());
        return 4;
    }
}
