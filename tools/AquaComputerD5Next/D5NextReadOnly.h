/*---------------------------------------------------------*\
| D5NextReadOnly.h                                          |
|                                                           |
|   Linux read-only D5 NEXT diagnostic helpers              |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "D5NextProtocol.h"

struct D5NextDescriptor
{
    unsigned int input_bits[256]   = {};
    unsigned int output_bits[256]  = {};
    unsigned int feature_bits[256] = {};
};

struct D5NextDeviceInfo
{
    std::filesystem::path hid_path;
    std::filesystem::path interface_path;
    std::filesystem::path usb_path;
    std::filesystem::path dev_node;
};

enum D5NextReadResult
{
    D5NEXT_READ_OK,
    D5NEXT_READ_PERMISSION_DENIED,
    D5NEXT_READ_COMPETING_PROCESS,
    D5NEXT_READ_DESCRIPTOR_MISMATCH,
    D5NEXT_READ_OPEN_FAILED,
    D5NEXT_READ_LOCK_FAILED,
    D5NEXT_READ_IDENTITY_FAILED,
    D5NEXT_READ_TRANSFER_FAILED,
    D5NEXT_READ_INVALID_REPORT,
    D5NEXT_READ_STATUS_FAILED,
};

class D5NextReadOnly
{
public:
    static bool ReadText(const std::filesystem::path& path, std::string& value);
    static bool ReadDescriptor(const std::filesystem::path& path, std::vector<unsigned char>& data);
    static bool ParseDescriptor(const std::vector<unsigned char>& data, D5NextDescriptor& descriptor);
    static std::vector<D5NextDeviceInfo> Discover(const std::filesystem::path& hidraw_class = "/sys/class/hidraw",
                                                 const std::filesystem::path& device_directory = "/dev");
    static std::vector<std::string> FindControllerProcesses(const std::filesystem::path& proc = "/proc");
    /*-----------------------------------------------------*\
    | At most one GET, no retry, and no device write API.   |
    | Optional firmware requires one passive Input01 first. |
    | The descriptor and permissions must pass first.      |
    | Process checks and flock are cooperative safeguards, |
    | not proof of exclusive ownership against all clients. |
    \*-----------------------------------------------------*/
    static D5NextReadResult GetFeatureOnce(const D5NextDeviceInfo& device, const D5NextDescriptor& descriptor,
                                          bool competing_process, unsigned char* report,
                                          int& received, int& system_error, D5NextResult& validation,
                                          unsigned int* firmware = nullptr);
    static const char* ReadResultString(D5NextReadResult result);
};
