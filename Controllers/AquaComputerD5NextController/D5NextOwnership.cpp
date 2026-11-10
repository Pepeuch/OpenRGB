/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "D5NextOwnership.h"
#include <set>

namespace
{
std::mutex registry_mutex;
std::set<std::string> reserved_paths;
std::set<std::wstring> reserved_serials;
}

void D5NextHIDCloser::operator()(hid_device* handle) const noexcept
{
    if(handle != nullptr)
    {
        hid_close(handle);
    }
}

D5NextOwnership::D5NextOwnership(std::string path_value, std::wstring serial_value)
    : path(std::move(path_value)), serial(std::move(serial_value))
{
}

std::unique_ptr<D5NextOwnership> D5NextOwnership::Acquire(const char* path, const wchar_t* serial)
{
    if(path == nullptr || path[0] == '\0')
    {
        return nullptr;
    }
    std::unique_ptr<D5NextOwnership> owner(new D5NextOwnership(path, serial == nullptr ? L"" : serial));
    std::lock_guard<std::mutex> lock(registry_mutex);
    if(reserved_paths.count(owner->path) || (!owner->serial.empty() && reserved_serials.count(owner->serial)))
    {
        return nullptr;
    }
    reserved_paths.insert(owner->path);
    try
    {
        if(!owner->serial.empty())
        {
            reserved_serials.insert(owner->serial);
        }
    }
    catch(...)
    {
        reserved_paths.erase(owner->path);
        throw;
    }
    owner->reserved = true;
    return owner;
}

D5NextOwnership::~D5NextOwnership() noexcept
{
    if(!reserved)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(registry_mutex);
    reserved_paths.erase(path);
    if(!serial.empty())
    {
        reserved_serials.erase(serial);
    }
}
