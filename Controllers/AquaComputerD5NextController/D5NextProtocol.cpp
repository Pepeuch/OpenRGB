/*---------------------------------------------------------*\
| D5NextProtocol.cpp                                        |
|                                                           |
|   Offline D5 NEXT lighting report codec                   |
|   Protocol references: Documentation/AquaComputerD5Next.md |
|                                                           |
|   OpenRGB contributors                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstring>
#include "CRC.h"
#include "D5NextProtocol.h"

unsigned short D5NextProtocol::CalculateCRC(const unsigned char* data, std::size_t size)
{
    /*-----------------------------------------------------*\
    | Use the existing CRC++ dependency, with the USB       |
    | parameters. No aquapx implementation is incorporated. |
    \*-----------------------------------------------------*/
    static const CRCPP::CRC::Parameters<unsigned short, 16> parameters =
    {
        0x8005, 0xFFFF, 0xFFFF, true, true
    };

    return CRCPP::CRC::Calculate(data, size, parameters);
}

D5NextResult D5NextProtocol::ValidateReport(const unsigned char* report, std::size_t size)
{
    if(report == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(size != D5NEXT_REPORT_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    if(report[0] != D5NEXT_REPORT_ID)
    {
        return D5NEXT_INVALID_REPORT_ID;
    }

    unsigned short stored = (unsigned short)((report[D5NEXT_CRC_OFFSET] << 8)
                                           | report[D5NEXT_CRC_OFFSET + 1]);
    if(CalculateCRC(report + 1, D5NEXT_CRC_OFFSET - 1) != stored)
    {
        return D5NEXT_INVALID_CRC;
    }
    return D5NEXT_OK;
}

bool D5NextProtocol::IsKnownEffect(unsigned int effect)
{
    /*-----------------------------------------------------*\
    | Only the three selectors described explicitly in     |
    | PROTOCOL.md are staged in this first implementation.  |
    \*-----------------------------------------------------*/
    return effect == D5NEXT_EFFECT_STATIC || effect == D5NEXT_EFFECT_BREATHING
        || effect == D5NEXT_EFFECT_RAINBOW;
}

bool D5NextProtocol::IsValidLighting(const D5NextLighting& lighting)
{
    return lighting.hue < D5NEXT_HUE_STEPS && lighting.saturation <= 255
        && lighting.brightness <= 255 && IsKnownEffect(lighting.effect)
        && (lighting.enabled == D5NEXT_LIGHTING_ON || lighting.enabled == D5NEXT_LIGHTING_OFF);
}

D5NextResult D5NextProtocol::Decode(const unsigned char* report, std::size_t size,
                                   D5NextLayout layout, D5NextLighting& lighting)
{
    D5NextResult result = ValidateReport(report, size);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(layout != D5NEXT_LAYOUT_DOCUMENTED_809)
    {
        return D5NEXT_UNSUPPORTED_LAYOUT;
    }

    D5NextLighting decoded;
    decoded.hue         = (report[D5NEXT_HUE_OFFSET] << 8) | report[D5NEXT_HUE_OFFSET + 1];
    decoded.saturation  = report[D5NEXT_SATURATION_OFFSET];
    decoded.brightness  = report[D5NEXT_BRIGHTNESS_OFFSET];
    decoded.enabled     = report[D5NEXT_ENABLE_OFFSET];
    decoded.effect      = report[D5NEXT_EFFECT_OFFSET];
    if(!IsValidLighting(decoded))
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    lighting = decoded;
    return D5NEXT_OK;
}

D5NextLighting D5NextProtocol::RGBToHSV(unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned int maximum = red;
    unsigned int minimum = red;
    if(green > maximum)
    {
        maximum = green;
    }
    if(blue > maximum)
    {
        maximum = blue;
    }
    if(green < minimum)
    {
        minimum = green;
    }
    if(blue < minimum)
    {
        minimum = blue;
    }

    unsigned int delta = maximum - minimum;
    D5NextLighting lighting = { 0, 0, maximum, D5NEXT_LIGHTING_ON, D5NEXT_EFFECT_STATIC };
    if(delta != 0)
    {
        /*-------------------------------------------------*\
        | Compute directly in six sectors of 256 hue steps. |
        | Avoid the existing integer-degree HSV helper's    |
        | loss of precision and undefined grayscale hue.    |
        \*-------------------------------------------------*/
        int numerator;
        if(maximum == red)
        {
            numerator = 256 * ((int)green - (int)blue);
        }
        else if(maximum == green)
        {
            numerator = 512 * (int)delta + 256 * ((int)blue - (int)red);
        }
        else
        {
            numerator = 1024 * (int)delta + 256 * ((int)red - (int)green);
        }
        if(numerator < 0)
        {
            numerator += D5NEXT_HUE_STEPS * (int)delta;
        }
        lighting.hue        = ((numerator + delta / 2) / delta) % D5NEXT_HUE_STEPS;
        lighting.saturation = (255 * delta + maximum / 2) / maximum;
    }
    return lighting;
}

bool D5NextProtocol::IsLightingOffset(std::size_t offset)
{
    return offset == D5NEXT_BRIGHTNESS_OFFSET || offset == D5NEXT_ENABLE_OFFSET
        || offset == D5NEXT_EFFECT_OFFSET || offset == D5NEXT_HUE_OFFSET
        || offset == D5NEXT_HUE_OFFSET + 1 || offset == D5NEXT_SATURATION_OFFSET;
}

D5NextResult D5NextProtocol::ValidateLightingChange(const unsigned char* original, std::size_t original_size,
                                                   const unsigned char* modified, std::size_t modified_size,
                                                   D5NextLayout layout)
{
    D5NextLighting lighting;
    D5NextResult result = Decode(original, original_size, layout, lighting);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    result = Decode(modified, modified_size, layout, lighting);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    for(std::size_t offset = 0; offset < D5NEXT_REPORT_SIZE; offset++)
    {
        if(original[offset] != modified[offset] && !IsLightingOffset(offset)
        && offset != D5NEXT_CRC_OFFSET && offset != D5NEXT_CRC_OFFSET + 1)
        {
            return D5NEXT_UNEXPECTED_CHANGE;
        }
    }
    return D5NEXT_OK;
}

D5NextResult D5NextProtocol::BuildLightingReport(const unsigned char* original, std::size_t original_size,
                                                 D5NextLayout layout, const D5NextLighting& lighting,
                                                 unsigned char* output, std::size_t output_size)
{
    if(output == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(output_size != D5NEXT_REPORT_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    D5NextLighting previous;
    D5NextResult result = Decode(original, original_size, layout, previous);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(!IsValidLighting(lighting))
    {
        return D5NEXT_INVALID_LIGHTING;
    }

    unsigned char candidate[D5NEXT_REPORT_SIZE];
    memcpy(candidate, original, sizeof(candidate));
    candidate[D5NEXT_BRIGHTNESS_OFFSET]  = (unsigned char)lighting.brightness;
    candidate[D5NEXT_ENABLE_OFFSET]      = (unsigned char)lighting.enabled;
    candidate[D5NEXT_EFFECT_OFFSET]      = (unsigned char)lighting.effect;
    candidate[D5NEXT_HUE_OFFSET]         = (unsigned char)(lighting.hue >> 8);
    candidate[D5NEXT_HUE_OFFSET + 1]     = (unsigned char)lighting.hue;
    candidate[D5NEXT_SATURATION_OFFSET] = (unsigned char)lighting.saturation;

    unsigned short crc = CalculateCRC(candidate + 1, D5NEXT_CRC_OFFSET - 1);
    candidate[D5NEXT_CRC_OFFSET]        = (unsigned char)(crc >> 8);
    candidate[D5NEXT_CRC_OFFSET + 1]    = (unsigned char)crc;

    result = ValidateLightingChange(original, original_size, candidate, sizeof(candidate), layout);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    bool unchanged = memcmp(original, candidate, sizeof(candidate)) == 0;
    memcpy(output, candidate, sizeof(candidate));
    return unchanged ? D5NEXT_UNCHANGED : D5NEXT_OK;
}

const char* D5NextProtocol::ResultString(D5NextResult result)
{
    switch(result)
    {
    case D5NEXT_OK:                  return "OK";
    case D5NEXT_UNCHANGED:           return "unchanged";
    case D5NEXT_INVALID_ARGUMENT:    return "invalid buffer";
    case D5NEXT_INVALID_SIZE:        return "unexpected report size";
    case D5NEXT_INVALID_REPORT_ID:   return "unexpected report ID";
    case D5NEXT_INVALID_CRC:         return "invalid CRC";
    case D5NEXT_UNSUPPORTED_LAYOUT:  return "unconfirmed firmware/layout";
    case D5NEXT_INVALID_LIGHTING:    return "unknown or invalid lighting fields";
    case D5NEXT_UNEXPECTED_CHANGE:   return "non-lighting byte changed";
    case D5NEXT_IO_ERROR:            return "feature report transfer failed";
    case D5NEXT_NOT_READY:           return "device unavailable or previous transfer failed";
    default:                        return "unknown error";
    }
}
