/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <hidapi.h>

struct D5NextHIDCloser
{
    void operator()(hid_device* handle) const noexcept;
};

using D5NextHIDHandle = std::unique_ptr<hid_device, D5NextHIDCloser>;

/* A reservation covers one in-process HID path and, when supplied, its USB
 * serial. The registry lock is never held during a HID transaction. */
class D5NextOwnership
{
public:
    static std::unique_ptr<D5NextOwnership> Acquire(const char* path, const wchar_t* serial);
    ~D5NextOwnership() noexcept;
    D5NextOwnership(const D5NextOwnership&) = delete;
    D5NextOwnership& operator=(const D5NextOwnership&) = delete;

    const std::string& Path() const noexcept { return path; }
    bool HasSerial() const noexcept { return !serial.empty(); }
    bool MatchesSerial(const wchar_t* value) const { return HasSerial() && value != nullptr && serial == value; }
    std::mutex transaction_mutex;

private:
    D5NextOwnership(std::string path_value, std::wstring serial_value);
    std::string path;
    std::wstring serial;
    bool reserved = false;
};
