/*---------------------------------------------------------*\
| D5NextReadOnly.cpp                                        |
|                                                           |
|   Linux read-only D5 NEXT diagnostic helpers              |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "D5NextReadOnly.h"

bool D5NextReadOnly::ReadText(const std::filesystem::path& path, std::string& value)
{
    std::ifstream stream(path);
    if(!stream || !std::getline(stream, value))
    {
        value.clear();
        return false;
    }
    return true;
}

bool D5NextReadOnly::ReadDescriptor(const std::filesystem::path& path, std::vector<unsigned char>& data)
{
    std::ifstream stream(path, std::ios::binary);
    data.clear();
    char byte;
    while(stream.get(byte))
    {
        if(data.size() == HID_MAX_DESCRIPTOR_SIZE)
        {
            data.clear();
            return false;
        }
        data.push_back((unsigned char)byte);
    }
    return stream.eof() && !data.empty();
}

bool D5NextReadOnly::ParseDescriptor(const std::vector<unsigned char>& data, D5NextDescriptor& descriptor)
{
    struct GlobalState
    {
        unsigned int size;
        unsigned int count;
        unsigned int id;
    };
    GlobalState globals = { 0, 0, 0 };
    std::vector<GlobalState> stack;
    D5NextDescriptor parsed;
    for(std::size_t offset = 0; offset < data.size();)
    {
        unsigned char prefix = data[offset++];
        if(prefix == 0xFE)
        {
            /* Unknown long items cannot safely establish a report shape. */
            return false;
        }
        unsigned int size = prefix & 3;
        if(size == 3)
        {
            size = 4;
        }
        if(size > data.size() - offset)
        {
            return false;
        }
        unsigned int value = 0;
        for(unsigned int byte = 0; byte < size; byte++)
        {
            value |= (unsigned int)data[offset++] << (8 * byte);
        }
        unsigned int type = (prefix >> 2) & 3;
        unsigned int tag  = prefix >> 4;
        if(type == 1)
        {
            switch(tag)
            {
            case 7: globals.size = value; break;
            case 8:
                if(value == 0 || value > 255)
                {
                    return false;
                }
                globals.id = value;
                break;
            case 9: globals.count = value; break;
            case 10:
                if(stack.size() >= 32)
                {
                    return false;
                }
                stack.push_back(globals);
                break;
            case 11:
                if(stack.empty())
                {
                    return false;
                }
                globals = stack.back();
                stack.pop_back();
                break;
            default: break;
            }
        }
        else if(type == 0 && (tag == 8 || tag == 9 || tag == 11))
        {
            if(globals.size == 0 || globals.size > 32 || globals.count == 0
            || globals.count > 65536 / globals.size)
            {
                return false;
            }
            unsigned int* bits = tag == 8 ? parsed.input_bits : (tag == 9 ? parsed.output_bits : parsed.feature_bits);
            unsigned int additional = globals.count * globals.size;
            if(bits[globals.id] > 65536 - additional)
            {
                return false;
            }
            bits[globals.id] += additional;
        }
    }
    if(data.empty() || !stack.empty())
    {
        return false;
    }
    descriptor = parsed;
    return true;
}

std::vector<D5NextDeviceInfo> D5NextReadOnly::Discover(const std::filesystem::path& hidraw_class,
                                                     const std::filesystem::path& device_directory)
{
    std::vector<D5NextDeviceInfo> devices;
    std::error_code error;
    std::filesystem::directory_iterator entries(hidraw_class, error);
    if(error)
    {
        return devices;
    }
    for(const std::filesystem::directory_entry& entry : entries)
    {
        D5NextDeviceInfo device;
        device.hid_path = std::filesystem::canonical(entry.path() / "device", error);
        if(error)
        {
            continue;
        }
        std::filesystem::path parent = device.hid_path;
        while(parent != parent.root_path())
        {
            std::string interface_number;
            if(ReadText(parent / "bInterfaceNumber", interface_number))
            {
                if(interface_number == "01")
                {
                    device.interface_path = parent;
                }
                break;
            }
            parent = parent.parent_path();
        }
        if(device.interface_path.empty())
        {
            continue;
        }
        device.usb_path = device.interface_path.parent_path();
        std::string vendor;
        std::string product;
        std::string interface_class;
        if(!ReadText(device.usb_path / "idVendor", vendor) || vendor != "0c70"
        || !ReadText(device.usb_path / "idProduct", product) || product != "f00e"
        || !ReadText(device.interface_path / "bInterfaceClass", interface_class) || interface_class != "03")
        {
            continue;
        }
        device.dev_node = device_directory / entry.path().filename();
        devices.push_back(device);
    }
    return devices;
}

std::vector<std::string> D5NextReadOnly::FindControllerProcesses(const std::filesystem::path& proc)
{
    std::vector<std::string> processes;
    std::error_code error;
    std::filesystem::directory_iterator entries(proc, error);
    if(error)
    {
        processes.push_back("process inventory unavailable");
        return processes;
    }
    for(const std::filesystem::directory_entry& entry : entries)
    {
        std::string pid = entry.path().filename().string();
        if(pid.empty() || pid.find_first_not_of("0123456789") != std::string::npos)
        {
            continue;
        }
        std::string name;
        if(!ReadText(entry.path() / "comm", name))
        {
            continue;
        }
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c)
        {
            return (char)std::tolower(c);
        });
        if(name.find("openrgb") != std::string::npos || name.find("aquapx") != std::string::npos
        || name.find("aquasuite") != std::string::npos || name.find("coolercontrol") != std::string::npos
        || name.find("liquidctl") != std::string::npos)
        {
            processes.push_back(pid + ":" + name);
        }
    }
    return processes;
}

D5NextReadResult D5NextReadOnly::GetFeatureOnce(const D5NextDeviceInfo& device, const D5NextDescriptor& descriptor,
                                              bool competing_process, unsigned char* report,
                                              int& received, int& system_error, D5NextResult& validation)
{
    received     = 0;
    system_error = 0;
    validation   = D5NEXT_NOT_READY;
    struct stat before;
    if(stat(device.dev_node.c_str(), &before) != 0 || !S_ISCHR(before.st_mode)
    || faccessat(AT_FDCWD, device.dev_node.c_str(), R_OK, AT_EACCESS) != 0)
    {
        system_error = errno;
        return D5NEXT_READ_PERMISSION_DENIED;
    }
    if(competing_process)
    {
        return D5NEXT_READ_COMPETING_PROCESS;
    }
    if(descriptor.feature_bits[3] != 808 * 8 || report == nullptr)
    {
        return D5NEXT_READ_DESCRIPTOR_MISMATCH;
    }
    int fd = open(device.dev_node.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if(fd < 0)
    {
        system_error = errno;
        return D5NEXT_READ_OPEN_FAILED;
    }
    if(flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        system_error = errno;
        close(fd);
        return D5NEXT_READ_LOCK_FAILED;
    }
    struct stat after;
    struct hidraw_devinfo identity = {};
    if(fstat(fd, &after) != 0 || !S_ISCHR(after.st_mode)
    || after.st_rdev != before.st_rdev || after.st_ino != before.st_ino
    || ioctl(fd, HIDIOCGRAWINFO, &identity) < 0 || identity.bustype != BUS_USB
    || identity.vendor != D5NEXT_VID || (unsigned short)identity.product != D5NEXT_PID)
    {
        system_error = errno;
        close(fd);
        return D5NEXT_READ_IDENTITY_FAILED;
    }
    unsigned char buffer[D5NEXT_REPORT_SIZE] = { D5NEXT_REPORT_ID };
    /*-----------------------------------------------------*\
    | The only USB request in this tool. No loop or retry.  |
    | No SET, Output, reset, status polling or save API.     |
    \*-----------------------------------------------------*/
    received = ioctl(fd, HIDIOCGFEATURE(sizeof(buffer)), buffer);
    system_error = received < 0 ? errno : 0;
    close(fd);
    if(received < 0)
    {
        return D5NEXT_READ_TRANSFER_FAILED;
    }
    validation = D5NextProtocol::ValidateReport(buffer, (std::size_t)received);
    if(validation != D5NEXT_OK)
    {
        return D5NEXT_READ_INVALID_REPORT;
    }
    memcpy(report, buffer, sizeof(buffer));
    return D5NEXT_READ_OK;
}

const char* D5NextReadOnly::ReadResultString(D5NextReadResult result)
{
    switch(result)
    {
    case D5NEXT_READ_OK:                    return "valid report read";
    case D5NEXT_READ_PERMISSION_DENIED:     return "node missing, not a character device, or read permission denied; not opened";
    case D5NEXT_READ_COMPETING_PROCESS:     return "possible competing controller; not opened";
    case D5NEXT_READ_DESCRIPTOR_MISMATCH:   return "feature 0x03 shape not confirmed by descriptor; not opened";
    case D5NEXT_READ_OPEN_FAILED:           return "read-only open failed; no retry";
    case D5NEXT_READ_LOCK_FAILED:           return "cooperative lock unavailable; no request sent";
    case D5NEXT_READ_IDENTITY_FAILED:       return "opened device identity changed or could not be verified; no feature request";
    case D5NEXT_READ_TRANSFER_FAILED:       return "GET_FEATURE failed; no retry";
    case D5NEXT_READ_INVALID_REPORT:        return "GET_FEATURE returned an invalid or unexpected report; no retry";
    default:                               return "unknown diagnostic result";
    }
}
