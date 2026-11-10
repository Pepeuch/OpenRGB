/*---------------------------------------------------------*\
| D5NextProtocol.h                                          |
|                                                           |
|   Offline D5 NEXT lighting report codec                   |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <cstddef>

#define D5NEXT_VID                   0x0C70
#define D5NEXT_PID                   0xF00E
#define D5NEXT_INTERFACE             1
#define D5NEXT_REPORT_ID             0x03
#define D5NEXT_REPORT_SIZE           809
#define D5NEXT_BRIGHTNESS_OFFSET     237
#define D5NEXT_ENABLE_OFFSET         239
#define D5NEXT_EFFECT_OFFSET         663
#define D5NEXT_HUE_OFFSET            706
#define D5NEXT_SATURATION_OFFSET     708
#define D5NEXT_CRC_OFFSET            807
#define D5NEXT_HUE_STEPS             1536

#define D5NEXT_EFFECT_STATIC         0x01
#define D5NEXT_EFFECT_BREATHING      0x02
#define D5NEXT_EFFECT_RAINBOW        0x03
#define D5NEXT_LIGHTING_ON           0x00
#define D5NEXT_LIGHTING_OFF          0x02

/*---------------------------------------------------------*\
| A layout is an explicit codec assumption, not firmware    |
| identification. No hardware firmware is approved yet.     |
\*---------------------------------------------------------*/
enum D5NextLayout
{
    D5NEXT_LAYOUT_UNKNOWN,
    D5NEXT_LAYOUT_DOCUMENTED_809,
};

enum D5NextResult
{
    D5NEXT_OK,
    D5NEXT_UNCHANGED,
    D5NEXT_INVALID_ARGUMENT,
    D5NEXT_INVALID_SIZE,
    D5NEXT_INVALID_REPORT_ID,
    D5NEXT_INVALID_CRC,
    D5NEXT_UNSUPPORTED_LAYOUT,
    D5NEXT_INVALID_LIGHTING,
    D5NEXT_UNEXPECTED_CHANGE,
    D5NEXT_IO_ERROR,
    D5NEXT_NOT_READY,
};

struct D5NextLighting
{
    unsigned int    hue;
    unsigned int    saturation;
    unsigned int    brightness;
    unsigned int    enabled;
    unsigned int    effect;
};

class D5NextProtocol
{
public:
    static unsigned short CalculateCRC(const unsigned char* data, std::size_t size);
    static D5NextResult ValidateReport(const unsigned char* report, std::size_t size);
    static D5NextResult Decode(const unsigned char* report, std::size_t size,
                              D5NextLayout layout, D5NextLighting& lighting);
    static D5NextLighting RGBToHSV(unsigned char red, unsigned char green, unsigned char blue);
    static bool IsKnownEffect(unsigned int effect);
    static bool IsLightingOffset(std::size_t offset);
    static D5NextResult ValidateLightingChange(const unsigned char* original, std::size_t original_size,
                                             const unsigned char* modified, std::size_t modified_size,
                                             D5NextLayout layout);
    /*-----------------------------------------------------*\
    | Output is untouched on failure. Input/output may     |
    | overlap. All non-lighting bytes are preserved.        |
    \*-----------------------------------------------------*/
    static D5NextResult BuildLightingReport(const unsigned char* original, std::size_t original_size,
                                           D5NextLayout layout, const D5NextLighting& lighting,
                                           unsigned char* output, std::size_t output_size);
    static const char* ResultString(D5NextResult result);

private:
    static bool IsValidLighting(const D5NextLighting& lighting);
};
