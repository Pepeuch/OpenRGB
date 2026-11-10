/*---------------------------------------------------------*\
| D5NextProtocol.cpp                                        |
|                                                           |
|   Offline D5 NEXT lighting report codec                   |
|   Protocol references: Documentation/AquaComputerD5Next.md |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
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
    if(report[1] != 0 || report[2] != 3)
    {
        return D5NEXT_UNSUPPORTED_STRUCTURE;
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
    decoded.enabled     = report[D5NEXT_ENABLE_OFFSET] & D5NEXT_LIGHTING_OFF;
    decoded.effect      = report[D5NEXT_EFFECT_OFFSET];
    if(!IsValidLighting(decoded))
    {
        return D5NEXT_INVALID_LIGHTING;
    }
    lighting = decoded;
    return D5NEXT_OK;
}

D5NextHSV D5NextProtocol::RGBToHSV(unsigned char red, unsigned char green, unsigned char blue)
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
    D5NextHSV lighting = { 0, 0, (unsigned char)maximum };
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
    case D5NEXT_UNSUPPORTED_FIRMWARE: return "unknown firmware";
    case D5NEXT_UNSUPPORTED_STRUCTURE:return "unknown configuration structure";
    case D5NEXT_INVALID_GROUP:       return "invalid RGBpx group";
    case D5NEXT_UNSUPPORTED_OPERATION:return "RGBpx operation not established by evidence";
    case D5NEXT_UNSUPPORTED_TRANSITION:return "mode transition requires reviewed preconditions";
    case D5NEXT_PRECONDITION_FAILED:  return "RGBpx patch precondition failed";
    default:                        return "unknown error";
    }
}
