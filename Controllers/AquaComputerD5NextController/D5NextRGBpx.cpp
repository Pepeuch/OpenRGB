/*---------------------------------------------------------*\
| D5NextRGBpx.cpp                                           |
|                                                           |
|   Explicit BE codecs and audited RGBpx edits              |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <climits>
#include <cstring>
#include "D5NextRGBpx.h"

static_assert(CHAR_BIT == 8 && USHRT_MAX == 65535 && SHRT_MIN == -32768 && SHRT_MAX == 32767,
              "D5 NEXT codecs require 8-bit bytes and 16-bit shorts");

D5NextResult D5NextRGBpx::ValidateIdentity(const D5NextIdentity& identity)
{
    if(identity.vendor != D5NEXT_VID || identity.product != D5NEXT_PID
    || identity.interface_number != D5NEXT_INTERFACE)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    return identity.firmware == D5NEXT_FIRMWARE ? D5NEXT_OK : D5NEXT_UNSUPPORTED_FIRMWARE;
}

bool D5NextRGBpx::ReadBE16(const unsigned char* bytes, std::size_t size, std::size_t offset,
                          unsigned short& value)
{
    if(bytes == nullptr || offset > size || size - offset < 2)
    {
        return false;
    }
    value = (unsigned short)((bytes[offset] << 8) | bytes[offset + 1]);
    return true;
}

bool D5NextRGBpx::ReadInt16(const unsigned char* bytes, std::size_t size, std::size_t offset, short& value)
{
    unsigned short raw;
    if(!ReadBE16(bytes, size, offset, raw))
    {
        return false;
    }
    /* Subtract in int, so conversion to short is always representable. */
    value = (short)((raw & 0x8000) ? (int)raw - 65536 : (int)raw);
    return true;
}

bool D5NextRGBpx::WriteBE16(unsigned char* bytes, std::size_t size, std::size_t offset, unsigned int value)
{
    if(bytes == nullptr || offset > size || size - offset < 2 || value > 65535)
    {
        return false;
    }
    bytes[offset]     = (unsigned char)(value >> 8);
    bytes[offset + 1] = (unsigned char)value;
    return true;
}

bool D5NextRGBpx::WriteInt16(unsigned char* bytes, std::size_t size, std::size_t offset, int value)
{
    if(value < -32768 || value > 32767)
    {
        return false;
    }
    return WriteBE16(bytes, size, offset, (unsigned int)value & 0xFFFF);
}

D5NextHSV D5NextRGBpx::RGBToHSV(unsigned char red, unsigned char green, unsigned char blue)
{
    /* Protocol conversion returns HSV Value as a separate typed field. */
    return D5NextProtocol::RGBToHSV(red, green, blue);
}

D5NextResult D5NextRGBpx::DecodeController(const unsigned char* bytes, std::size_t size,
                                          D5NextRGBpxController& controller)
{
    if(bytes == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(size != D5NEXT_GROUP_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    D5NextRGBpxController decoded = {};
    decoded.strip     = bytes[0];
    decoded.led_start = bytes[1];
    decoded.count     = bytes[2];
    decoded.mode      = bytes[3];
    ReadBE16(bytes, size, 4, decoded.flags);
    ReadInt16(bytes, size, 6, decoded.source_id);
    decoded.source_filter[0] = bytes[8];
    decoded.source_filter[1] = bytes[9];
    for(unsigned int index = 0; index < 2; index++)
    {
        unsigned int offset = 10 + 6 * index;
        ReadInt16(bytes, size, offset,     decoded.binding[index].x1);
        ReadInt16(bytes, size, offset + 2, decoded.binding[index].x2);
        decoded.binding[index].y1 = bytes[offset + 4];
        decoded.binding[index].y2 = bytes[offset + 5];
    }
    for(unsigned int index = 0; index < D5NEXT_PARAMETER_COUNT; index++)
    {
        ReadInt16(bytes, size, 22 + 2 * index, decoded.values[index]);
    }
    for(unsigned int index = 0; index < D5NEXT_COLOR_COUNT; index++)
    {
        unsigned int offset = 46 + 4 * index;
        ReadBE16(bytes, size, offset, decoded.colors[index].hue);
        decoded.colors[index].saturation = bytes[offset + 2];
        decoded.colors[index].value      = bytes[offset + 3];
    }
    controller = decoded;
    return D5NEXT_OK;
}

D5NextResult D5NextRGBpx::EncodeController(const D5NextRGBpxController& controller,
                                          unsigned char* bytes, std::size_t size)
{
    if(bytes == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(size != D5NEXT_GROUP_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    for(unsigned int index = 0; index < D5NEXT_COLOR_COUNT; index++)
    {
        if(controller.colors[index].hue >= D5NEXT_HUE_STEPS)
        {
            return D5NEXT_INVALID_LIGHTING;
        }
    }
    unsigned char encoded[D5NEXT_GROUP_SIZE];
    encoded[0] = controller.strip;
    encoded[1] = controller.led_start;
    encoded[2] = controller.count;
    encoded[3] = controller.mode;
    WriteBE16(encoded, sizeof(encoded), 4, controller.flags);
    WriteInt16(encoded, sizeof(encoded), 6, controller.source_id);
    encoded[8] = controller.source_filter[0];
    encoded[9] = controller.source_filter[1];
    for(unsigned int index = 0; index < 2; index++)
    {
        unsigned int offset = 10 + 6 * index;
        WriteInt16(encoded, sizeof(encoded), offset,     controller.binding[index].x1);
        WriteInt16(encoded, sizeof(encoded), offset + 2, controller.binding[index].x2);
        encoded[offset + 4] = controller.binding[index].y1;
        encoded[offset + 5] = controller.binding[index].y2;
    }
    for(unsigned int index = 0; index < D5NEXT_PARAMETER_COUNT; index++)
    {
        WriteInt16(encoded, sizeof(encoded), 22 + 2 * index, controller.values[index]);
    }
    for(unsigned int index = 0; index < D5NEXT_COLOR_COUNT; index++)
    {
        unsigned int offset = 46 + 4 * index;
        WriteBE16(encoded, sizeof(encoded), offset, controller.colors[index].hue);
        encoded[offset + 2] = controller.colors[index].saturation;
        encoded[offset + 3] = controller.colors[index].value;
    }
    memcpy(bytes, encoded, sizeof(encoded));
    return D5NEXT_OK;
}

D5NextResult D5NextRGBpx::Decode(const unsigned char* report, std::size_t size,
                                const D5NextIdentity& identity, D5NextRGBpxConfiguration& configuration)
{
    D5NextResult result = ValidateIdentity(identity);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    result = D5NextProtocol::ValidateReport(report, size);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    D5NextRGBpxConfiguration decoded = {};
    ReadBE16(report, size, 1, decoded.structure);
    if(decoded.structure != D5NEXT_STRUCTURE)
    {
        return D5NEXT_UNSUPPORTED_STRUCTURE;
    }
    decoded.master = report[237];
    ReadBE16(report, size, 238, decoded.strip_flags);
    for(unsigned int index = 0; index < D5NEXT_GROUP_COUNT; index++)
    {
        DecodeController(report + D5NEXT_GROUP_OFFSET + index * D5NEXT_GROUP_SIZE,
                         D5NEXT_GROUP_SIZE, decoded.groups[index]);
    }
    configuration = decoded;
    return D5NEXT_OK;
}

/* Separate reviewed policy: adding documentary metadata must never enable an
 * edit. These are the existing lot-6 offline patch modes, not hardware approval. */
static bool HasReviewedPatchPolicy(unsigned int mode)
{
    switch(mode)
    {
    case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
    case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x0C: case 0x0E: case 0x13:
        return true;
    default:
        return false;
    }
}

D5NextResult D5NextRGBpx::BuildPatch(const unsigned char* original, std::size_t size,
                                    const D5NextIdentity& identity, const D5NextRGBpxPatch& patch,
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
    D5NextRGBpxConfiguration configuration;
    D5NextResult result = Decode(original, size, identity, configuration);
    if(result != D5NEXT_OK)
    {
        return result;
    }
    if(patch.group >= D5NEXT_GROUP_COUNT)
    {
        return D5NEXT_INVALID_GROUP;
    }
    /* No allocation changes or edits to groups 0..5/7. Group 7 may be active. */
    if(patch.group != D5NEXT_INTERNAL_GROUP)
    {
        return D5NEXT_UNSUPPORTED_OPERATION;
    }
    const D5NextRGBpxController& group = configuration.groups[patch.group];
    if(group.strip != 1 || group.led_start != 0 || group.count != 6
    || patch.expected_mode != group.mode)
    {
        return D5NEXT_PRECONDITION_FAILED;
    }
    const D5NextEffectInfo* effect = D5NextEffects::Find(group.mode);
    if(!HasReviewedPatchPolicy(group.mode) || effect == nullptr
    || !effect->captured || effect->requires_pc_transport)
    {
        return D5NEXT_UNSUPPORTED_OPERATION;
    }

    unsigned int base = D5NEXT_GROUP_OFFSET + patch.group * D5NEXT_GROUP_SIZE;
    unsigned char candidate[D5NEXT_REPORT_SIZE];
    unsigned char allowed[D5NEXT_REPORT_SIZE] = {};
    memcpy(candidate, original, sizeof(candidate));
    switch(patch.kind)
    {
    case D5NEXT_PATCH_MASTER:
        if(patch.value < 0 || patch.value > 255)
        {
            return D5NEXT_INVALID_LIGHTING;
        }
        candidate[237] = (unsigned char)patch.value;
        allowed[237]   = 0xFF;
        break;

    case D5NEXT_PATCH_STRIP_OFF:
        candidate[239] = (unsigned char)((candidate[239] & ~D5NEXT_STRIP_OFF_MASK)
                         | (patch.enabled ? D5NEXT_STRIP_OFF_MASK : 0));
        allowed[239] = D5NEXT_STRIP_OFF_MASK;
        break;

    case D5NEXT_PATCH_PARAMETER:
    {
        const D5NextParameterInfo* parameter = D5NextEffects::Parameter(group.mode, patch.index);
        if(parameter == nullptr || parameter->evidence != D5NEXT_ISOLATED)
        {
            return D5NEXT_UNSUPPORTED_OPERATION;
        }
        bool observed = false;
        for(unsigned int index = 0; index < parameter->observed_count; index++)
        {
            observed |= patch.value == parameter->observed_values[index];
        }
        /* Observed samples are a conservative edit policy, not UI bounds. */
        if(!observed)
        {
            return D5NEXT_PRECONDITION_FAILED;
        }
        unsigned int offset = base + 22 + 2 * patch.index;
        if(!WriteInt16(candidate, sizeof(candidate), offset, patch.value))
        {
            return D5NEXT_INVALID_LIGHTING;
        }
        allowed[offset] = allowed[offset + 1] = 0xFF;
        break;
    }

    case D5NEXT_PATCH_EFFECT_FLAG:
    {
        const D5NextFlagInfo* flag = D5NextEffects::Flag(group.mode, patch.mask);
        if(flag == nullptr)
        {
            return D5NEXT_UNSUPPORTED_OPERATION;
        }
        unsigned short flags = (unsigned short)((group.flags & ~flag->mask)
                                | (patch.enabled ? flag->mask : 0));
        WriteBE16(candidate, sizeof(candidate), base + 4, flags);
        allowed[base + 4] = (unsigned char)(flag->mask >> 8);
        allowed[base + 5] = (unsigned char)flag->mask;
        break;
    }

    case D5NEXT_PATCH_COLOR:
    {
        const D5NextColorInfo* color = D5NextEffects::Color(group.mode, patch.index);
        if(color == nullptr || patch.mask == 0 || (patch.mask & ~color->components) != 0)
        {
            return D5NEXT_UNSUPPORTED_OPERATION;
        }
        if(((patch.mask & D5NEXT_COLOR_H) && patch.hue >= D5NEXT_HUE_STEPS)
        || ((patch.mask & D5NEXT_COLOR_S) && patch.saturation > 255)
        || ((patch.mask & D5NEXT_COLOR_V) && patch.hsv_value > 255))
        {
            return D5NEXT_INVALID_LIGHTING;
        }
        unsigned int offset = base + 46 + 4 * patch.index;
        if(patch.mask & D5NEXT_COLOR_H)
        {
            WriteBE16(candidate, sizeof(candidate), offset, patch.hue);
            allowed[offset] = allowed[offset + 1] = 0xFF;
        }
        if(patch.mask & D5NEXT_COLOR_S)
        {
            candidate[offset + 2] = (unsigned char)patch.saturation;
            allowed[offset + 2]   = 0xFF;
        }
        if(patch.mask & D5NEXT_COLOR_V)
        {
            candidate[offset + 3] = (unsigned char)patch.hsv_value;
            allowed[offset + 3]   = 0xFF;
        }
        break;
    }

    case D5NEXT_PATCH_MODE:
        if(patch.value != group.mode)
        {
            /* Captured initial values are not universal presets. */
            return D5NEXT_UNSUPPORTED_TRANSITION;
        }
        break;

    default:
        return D5NEXT_INVALID_ARGUMENT;
    }

    unsigned short crc = D5NextProtocol::CalculateCRC(candidate + 1, 806);
    WriteBE16(candidate, sizeof(candidate), 807, crc);
    allowed[807] = allowed[808] = 0xFF;
    for(unsigned int offset = 0; offset < D5NEXT_REPORT_SIZE; offset++)
    {
        if(((original[offset] ^ candidate[offset]) & ~allowed[offset]) != 0)
        {
            return D5NEXT_UNEXPECTED_CHANGE;
        }
    }
    result = memcmp(original, candidate, sizeof(candidate)) == 0 ? D5NEXT_UNCHANGED : D5NEXT_OK;
    memcpy(output, candidate, sizeof(candidate));
    return result;
}

D5NextResult D5NextRGBpx::AuditPatch(const unsigned char* original, std::size_t size,
                                    const D5NextIdentity& identity, const D5NextRGBpxPatch& patch,
                                    const unsigned char* candidate, std::size_t candidate_size)
{
    if(candidate == nullptr)
    {
        return D5NEXT_INVALID_ARGUMENT;
    }
    if(candidate_size != D5NEXT_REPORT_SIZE)
    {
        return D5NEXT_INVALID_SIZE;
    }
    unsigned char expected[D5NEXT_REPORT_SIZE];
    D5NextResult result = BuildPatch(original, size, identity, patch, expected, sizeof(expected));
    if(result != D5NEXT_OK && result != D5NEXT_UNCHANGED)
    {
        return result;
    }
    return memcmp(candidate, expected, sizeof(expected)) == 0 ? D5NEXT_OK : D5NEXT_UNEXPECTED_CHANGE;
}
