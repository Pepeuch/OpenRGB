/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <string>
#include "D5NextRGBpx.h"

namespace D5NextSpeedTest
{
enum class Action { Speed, BlueHue, ExternalBlueHue, Group7YellowHue };
std::string Digest(const unsigned char* bytes, std::size_t size);
D5NextResult Plan(const unsigned char* original, std::size_t size,
                  unsigned char (&candidate)[809], Action action = Action::Speed);
void PrintPlan(const unsigned char (&original)[809], const unsigned char (&candidate)[809], Action action = Action::Speed);

/* Dedicated tool only. Normal OpenRGB never links this transport or runner. */
class Transport
{
public:
    virtual ~Transport() = default;
    virtual int Get(unsigned char (&report)[809]) = 0;
    virtual int Set(const unsigned char (&report)[809]) = 0;
    virtual bool Safe() = 0;
};
enum class Mode { DryRun, SendOnce };
/* One fresh GET; zero/one SET; zero/one control GET. No retry or recovery. */
int Run(Transport& transport, Mode mode, const std::string& expected_digest,
        std::string& fresh_digest, Action action = Action::Speed);
}
