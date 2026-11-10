/*---------------------------------------------------------*\
| OfflineLog.cpp                                            |
|                                                           |
|   Test-only logging sink; no ResourceManager startup      |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "LogManager.h"

LogManager::LogManager()
{
}

LogManager::~LogManager()
{
}

LogManager* LogManager::get()
{
    static LogManager log;
    return &log;
}

void LogManager::LogEntry(const char* /*filename*/, int /*line*/, unsigned int /*level*/, const char* /*fmt*/, ...)
{
}
