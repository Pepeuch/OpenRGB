/*---------------------------------------------------------*\
| RGBpxTests.cpp                                            |
|                                                           |
|   Synthetic fixtures and independent documentary checks  |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
#include "D5NextRGBpx.h"
#include "D5NextSystemCommands.h"
#include "DocumentaryBulk.h"

static unsigned int checks;
#define CHECK(condition) Check((condition), #condition, __LINE__)
static void Check(bool condition, const char* expression, int line)
{
    checks++;
    if(!condition)
    {
        std::fprintf(stderr, "FAIL RGBpx line %d: %s\n", line, expression);
        std::exit(1);
    }
}

static D5NextIdentity identity = { 0x0C70, 0xF00E, 1, 1027 };

/* Independent reflected bit-stream oracle. No captured configuration. */
static unsigned short CRC(const unsigned char* data, std::size_t size)
{
    unsigned int value = 0xFFFF;
    for(std::size_t index = 0; index < size; index++)
    {
        value ^= data[index];
        for(unsigned int bit = 0; bit < 8; bit++)
        {
            value = (value >> 1) ^ ((value & 1) ? 0xA001 : 0);
        }
    }
    return (unsigned short)(value ^ 0xFFFF);
}

static void FixCRC(unsigned char* data)
{
    unsigned short crc = CRC(data + 1, 806);
    data[807] = (unsigned char)(crc >> 8);
    data[808] = (unsigned char)crc;
}

/* Offsets here are literal, deliberately independent of the encoder. */
static void Fixture(unsigned char* data, unsigned int mode = 0x13, unsigned int seed = 17)
{
    for(unsigned int index = 0; index < 809; index++)
    {
        data[index] = (unsigned char)(seed + index * 37);
    }
    data[0] = 3;
    data[1] = 0;
    data[2] = 3;
    data[237] = 153;
    data[238] = 0xA5;
    data[239] = 0xE0;
    for(unsigned int group = 0; group < 8; group++)
    {
        unsigned int base = 240 + 70 * group;
        data[base]     = group < 6 ? 0 : 1;
        data[base + 1] = group < 6 ? group * 15 : 0;
        data[base + 2] = group < 6 ? 15 : 6;
        data[base + 3] = group == 6 ? mode : (group == 7 ? 0 : 3);
        data[base + 4] = 0xB5;
        data[base + 5] = 0x01;
        data[base + 6] = 0xFF;
        data[base + 7] = 0xFF;
        data[base + 8] = 10;
        data[base + 9] = 15;
        const unsigned char bindings[] = { 0x80, 0, 0x7F, 0xFF, 7, 201, 0xFF, 0xFE, 0, 100, 9, 203 };
        memcpy(data + base + 10, bindings, sizeof(bindings));
        for(unsigned int index = 0; index < 12; index++)
        {
            int value = index % 2 ? -(int)(index + group + 1) : (int)(index + group + 1);
            unsigned int raw = (unsigned int)value & 0xFFFF;
            data[base + 22 + 2 * index] = (unsigned char)(raw >> 8);
            data[base + 23 + 2 * index] = (unsigned char)raw;
        }
        for(unsigned int index = 0; index < 6; index++)
        {
            unsigned int hue = index == 5 ? 1535 : 256 * index;
            data[base + 46 + 4 * index] = (unsigned char)(hue >> 8);
            data[base + 47 + 4 * index] = (unsigned char)hue;
            data[base + 48 + 4 * index] = 200 + index;
            data[base + 49 + 4 * index] = 11 + index;
        }
    }
    FixCRC(data);
}

static D5NextRGBpxPatch Patch(D5NextPatchKind kind, unsigned int mode)
{
    D5NextRGBpxPatch patch = {};
    patch.kind          = kind;
    patch.group         = 6;
    patch.expected_mode = mode;
    return patch;
}

static void TestEndian()
{
    unsigned char bytes[4] = { 0xA5, 0, 0, 0x5A };
    for(unsigned int raw = 0; raw <= 65535; raw++)
    {
        CHECK(D5NextRGBpx::WriteBE16(bytes, 4, 1, raw));
        CHECK(bytes[1] == (raw >> 8) && bytes[2] == (raw & 255));
        unsigned short decoded = 0;
        short signed_value = 0;
        CHECK(D5NextRGBpx::ReadBE16(bytes, 4, 1, decoded) && decoded == raw);
        CHECK(D5NextRGBpx::ReadInt16(bytes, 4, 1, signed_value));
        int expected = raw < 32768 ? (int)raw : (int)raw - 65536;
        CHECK(signed_value == expected);
        CHECK(D5NextRGBpx::WriteInt16(bytes, 4, 1, expected));
        CHECK(bytes[1] == (raw >> 8) && bytes[2] == (raw & 255));
        CHECK(bytes[0] == 0xA5 && bytes[3] == 0x5A);
    }
    unsigned char before[4];
    memcpy(before, bytes, sizeof(bytes));
    short signed_value = 123;
    unsigned short value = 456;
    const std::size_t offsets[] = { 3, 4, 5, std::numeric_limits<std::size_t>::max() };
    for(unsigned int index = 0; index < 4; index++)
    {
        CHECK(!D5NextRGBpx::WriteBE16(bytes, 4, offsets[index], 1));
        CHECK(!D5NextRGBpx::ReadBE16(bytes, 4, offsets[index], value) && value == 456);
        CHECK(!D5NextRGBpx::ReadInt16(bytes, 4, offsets[index], signed_value) && signed_value == 123);
        CHECK(memcmp(bytes, before, sizeof(bytes)) == 0);
    }
    CHECK(!D5NextRGBpx::WriteBE16(bytes, 4, 0, 65536));
    CHECK(!D5NextRGBpx::WriteInt16(bytes, 4, 0, -32769));
    CHECK(!D5NextRGBpx::WriteInt16(bytes, 4, 0, 32768));
    CHECK(!D5NextRGBpx::WriteInt16(nullptr, 4, 0, 0));
    CHECK(!D5NextRGBpx::ReadBE16(nullptr, 4, 0, value));
    CHECK(memcmp(bytes, before, sizeof(bytes)) == 0);
}

static void TestModel()
{
    unsigned char report[809];
    for(unsigned int seed = 0; seed < 16; seed++)
    {
        Fixture(report, 0x13, seed);
        D5NextRGBpxConfiguration configuration;
        CHECK(D5NextRGBpx::Decode(report, 809, identity, configuration) == D5NEXT_OK);
        CHECK(configuration.structure == 3 && configuration.master == 153 && configuration.strip_flags == 0xA5E0);
        for(unsigned int group = 0; group < 8; group++)
        {
            const D5NextRGBpxController& decoded = configuration.groups[group];
            CHECK(decoded.strip == (group < 6 ? 0 : 1));
            CHECK(decoded.led_start == (group < 6 ? group * 15 : 0));
            CHECK(decoded.count == (group < 6 ? 15 : 6));
            CHECK(decoded.flags == 0xB501 && decoded.source_id == -1);
            CHECK(decoded.source_filter[0] == 10 && decoded.source_filter[1] == 15);
            CHECK(decoded.binding[0].x1 == -32768 && decoded.binding[0].x2 == 32767);
            CHECK(decoded.binding[0].y1 == 7 && decoded.binding[0].y2 == 201);
            CHECK(decoded.binding[1].x1 == -2 && decoded.binding[1].x2 == 100);
            CHECK(decoded.binding[1].y1 == 9 && decoded.binding[1].y2 == 203);
            for(unsigned int index = 0; index < 12; index++)
            {
                CHECK(decoded.values[index] == (index % 2 ? -(int)(index + group + 1) : (int)(index + group + 1)));
            }
            for(unsigned int index = 0; index < 6; index++)
            {
                CHECK(decoded.colors[index].hue == (index == 5 ? 1535 : 256 * index));
                CHECK(decoded.colors[index].saturation == 200 + index && decoded.colors[index].value == 11 + index);
            }
            unsigned char encoded[70];
            CHECK(D5NextRGBpx::EncodeController(decoded, encoded, 70) == D5NEXT_OK);
            CHECK(memcmp(encoded, report + 240 + group * 70, 70) == 0);
        }
    }
    D5NextRGBpxConfiguration configuration;
    CHECK(D5NextRGBpx::Decode(report, 809, identity, configuration) == D5NEXT_OK);
    unsigned char encoded[70];
    memset(encoded, 0xA5, sizeof(encoded));
    configuration.groups[6].colors[5].hue = 1536;
    CHECK(D5NextRGBpx::EncodeController(configuration.groups[6], encoded, 70) == D5NEXT_INVALID_LIGHTING);
    for(unsigned int index = 0; index < 70; index++)
    {
        CHECK(encoded[index] == 0xA5);
    }
    CHECK(D5NextRGBpx::EncodeController(configuration.groups[6], encoded, 69) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextRGBpx::EncodeController(configuration.groups[6], nullptr, 70) == D5NEXT_INVALID_ARGUMENT);
    D5NextRGBpxController group = {};
    group.mode = 81;
    CHECK(D5NextRGBpx::DecodeController(encoded, 69, group) == D5NEXT_INVALID_SIZE && group.mode == 81);
    CHECK(D5NextRGBpx::DecodeController(nullptr, 70, group) == D5NEXT_INVALID_ARGUMENT && group.mode == 81);
    report[663] = 6;
    report[706] = 0xFF;
    FixCRC(report);
    CHECK(D5NextRGBpx::Decode(report, 809, identity, configuration) == D5NEXT_OK);
    CHECK(configuration.groups[6].mode == 6 && configuration.groups[6].colors[0].hue > 1535);
    D5NextHSV pink = D5NextRGBpx::RGBToHSV(255, 105, 180);
    CHECK(pink.hue == 1408 && pink.saturation == 150 && pink.value == 255);
}

static void TestLiteralControllerVector()
{
    /* Independent, fixed 70-byte vector: header, two bindings, 12 signed
     * parameters, and six HSV quadruplets. No fixture or encoder builds it. */
    const unsigned char bytes[70] = {
        1,0,6,4,0x12,0x34,0xFF,0xFE,0xAB,0xCD,
        0x80,0,0x7F,0xFF,9,10, 0xFF,0xFF,0,1,11,12,
        0x80,0,0x7F,0xFF,0xFF,0xFE,0,0,0,1,0xFF,0xFF,
        0x12,0x34,0xED,0xCC,0,10,0xFF,0xF6,0x05,0xFF,0,0,
        0x05,0x80,150,255, 0,0,0,0, 0,1,255,1,
        0x05,0xFF,1,2, 0,0,3,4, 0,0,5,6
    };
    D5NextRGBpxController decoded = {};
    CHECK(D5NextRGBpx::DecodeController(bytes, sizeof(bytes), decoded) == D5NEXT_OK);
    CHECK(decoded.strip == 1 && decoded.count == 6 && decoded.mode == 4);
    CHECK(decoded.flags == 0x1234 && decoded.source_id == -2);
    CHECK(decoded.source_filter[0] == 0xAB && decoded.source_filter[1] == 0xCD);
    CHECK(decoded.binding[0].x1 == -32768 && decoded.binding[0].x2 == 32767);
    CHECK(decoded.binding[0].y1 == 9 && decoded.binding[0].y2 == 10);
    CHECK(decoded.binding[1].x1 == -1 && decoded.binding[1].x2 == 1);
    CHECK(decoded.values[0] == -32768 && decoded.values[1] == 32767);
    CHECK(decoded.values[2] == -2 && decoded.values[7] == -4660);
    CHECK(decoded.values[9] == -10 && decoded.values[10] == 1535);
    CHECK(decoded.colors[0].hue == 1408 && decoded.colors[0].saturation == 150
          && decoded.colors[0].value == 255);
    CHECK(decoded.colors[3].hue == 1535 && decoded.colors[3].value == 2);
    unsigned char encoded[70];
    CHECK(D5NextRGBpx::EncodeController(decoded, encoded, sizeof(encoded)) == D5NEXT_OK);
    CHECK(memcmp(bytes, encoded, sizeof(bytes)) == 0);
}

static void Reject(const unsigned char* original, std::size_t size, const D5NextIdentity& device,
                   const D5NextRGBpxPatch& patch, D5NextResult expected)
{
    unsigned char output[809];
    memset(output, 0xA5, sizeof(output));
    CHECK(D5NextRGBpx::BuildPatch(original, size, device, patch, output, 809) == expected);
    for(unsigned int index = 0; index < 809; index++)
    {
        CHECK(output[index] == 0xA5);
    }
}

static void TestPatches()
{
    unsigned char original[809];
    unsigned char output[809];
    Fixture(original);
    D5NextRGBpxPatch patch = Patch(D5NEXT_PATCH_PARAMETER, 0x13);
    patch.index = 0;
    patch.value = 44;
    CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(output[682] == 0 && output[683] == 44);
    for(unsigned int offset = 0; offset < 807; offset++)
    {
        if(offset != 682 && offset != 683)
        {
            CHECK(output[offset] == original[offset]);
        }
    }
    CHECK(CRC(output + 1, 806) == (unsigned short)((output[807] << 8) | output[808]));
    CHECK(D5NextRGBpx::AuditPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(D5NextRGBpx::BuildPatch(output, 809, identity, patch, output, 809) == D5NEXT_UNCHANGED);
    unsigned char overlap[810];
    memcpy(overlap, original, 809);
    CHECK(D5NextRGBpx::BuildPatch(overlap, 809, identity, patch, overlap + 1, 809) == D5NEXT_OK);
    CHECK(memcmp(overlap + 1, output, 809) == 0);

    /* An attacker/bug changing any byte cannot hide behind a new valid CRC. */
    for(unsigned int offset = 0; offset < 809; offset++)
    {
        unsigned char corrupted[809];
        memcpy(corrupted, output, 809);
        corrupted[offset] ^= 1;
        if(offset > 0 && offset < 807)
        {
            FixCRC(corrupted);
        }
        CHECK(D5NextRGBpx::AuditPatch(original, 809, identity, patch, corrupted, 809) != D5NEXT_OK);
    }

    patch.value = -32768;
    Reject(original, 809, identity, patch, D5NEXT_PRECONDITION_FAILED);
    patch.value = 45; /* In the storage range, but not a validated UI bound/sample. */
    Reject(original, 809, identity, patch, D5NEXT_PRECONDITION_FAILED);
    patch.value = 44;
    patch.index = 12;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    patch.index = 0;
    for(unsigned int group = 0; group < 9; group++)
    {
        if(group == 6) continue;
        patch.group = group;
        Reject(original, 809, identity, patch, group == 8 ? D5NEXT_INVALID_GROUP : D5NEXT_UNSUPPORTED_OPERATION);
    }
    patch.group = 6;
    patch.expected_mode = 3;
    Reject(original, 809, identity, patch, D5NEXT_PRECONDITION_FAILED);
    patch = Patch(D5NEXT_PATCH_MODE, 0x13);
    patch.value = 1;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_TRANSITION);
    Fixture(original, 3);
    patch = Patch(D5NEXT_PATCH_MODE, 3);
    patch.value = 1;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_TRANSITION);
    patch.value = 3;
    CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_UNCHANGED);

    /* Lot 6B I1: old audit accepted an isolated Rainbow H edit while its
     * builder refused. The sole policy now refuses both build and audit. */
    patch = Patch(D5NEXT_PATCH_COLOR, 3);
    patch.mask = D5NEXT_COLOR_H;
    patch.hue = 1408;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    memcpy(output, original, sizeof(output));
    output[706] = 0x05;
    output[707] = 0x80;
    FixCRC(output);
    CHECK(D5NextRGBpx::AuditPatch(original, 809, identity, patch, output, 809)
          == D5NEXT_UNSUPPORTED_OPERATION);

    /* Old format-only builder overlooked internal-group strip allocation. */
    Fixture(original, 1);
    original[660] = 0;
    FixCRC(original);
    patch = Patch(D5NEXT_PATCH_MASTER, 1);
    patch.value = 200;
    Reject(original, 809, identity, patch, D5NEXT_PRECONDITION_FAILED);
    memcpy(output, original, sizeof(output));
    output[237] = 200;
    FixCRC(output);
    CHECK(D5NextRGBpx::AuditPatch(original, 809, identity, patch, output, 809)
          == D5NEXT_PRECONDITION_FAILED);

    Fixture(original, 4);
    patch = Patch(D5NEXT_PATCH_COLOR, 4);
    patch.mask = D5NEXT_COLOR_H | D5NEXT_COLOR_S;
    patch.index = 0; /* Blink1.Background, per documentary sample. */
    patch.hue = 1408;
    patch.saturation = 150;
    CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(output[706] == 0x05 && output[707] == 0x80 && output[708] == 150);
    CHECK(output[709] == original[709] && output[237] == original[237]);
    for(unsigned int offset = 0; offset < 807; offset++)
    {
        CHECK(offset == 706 || offset == 707 || offset == 708 || output[offset] == original[offset]);
    }
    CHECK(D5NextRGBpx::AuditPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    Fixture(original, 3);

    patch = Patch(D5NEXT_PATCH_MASTER, 3);
    const int masters[] = { 0, 153, 255 };
    for(unsigned int index = 0; index < 3; index++)
    {
        patch.value = masters[index];
        D5NextResult result = D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809);
        CHECK(result == (masters[index] == 153 ? D5NEXT_UNCHANGED : D5NEXT_OK));
        CHECK(output[237] == masters[index]);
        for(unsigned int offset = 0; offset < 807; offset++)
        {
            CHECK(offset == 237 || output[offset] == original[offset]);
        }
        CHECK(output[709] == 11 && output[238] == 0xA5 && output[239] == 0xE0);
    }
    patch.value = -1;
    Reject(original, 809, identity, patch, D5NEXT_INVALID_LIGHTING);
    patch.value = 256;
    Reject(original, 809, identity, patch, D5NEXT_INVALID_LIGHTING);
    patch = Patch(D5NEXT_PATCH_STRIP_OFF, 3);
    patch.enabled = true;
    CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(output[238] == 0xA5 && output[239] == 0xE2 && output[237] == 153 && output[709] == 11);
    patch.enabled = false;
    CHECK(D5NextRGBpx::BuildPatch(output, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(memcmp(original, output, 809) == 0);

    /* Same mask, different meaning, and independent strip/controller flags. */
    const unsigned int modes[] = { 3, 4 };
    for(unsigned int index = 0; index < 2; index++)
    {
        Fixture(original, modes[index]);
        patch = Patch(D5NEXT_PATCH_EFFECT_FLAG, modes[index]);
        patch.mask = 2;
        patch.enabled = true;
        CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
        CHECK(output[664] == 0xB5 && output[665] == 3 && output[239] == 0xE0);
        patch.enabled = false;
        CHECK(D5NextRGBpx::BuildPatch(output, 809, identity, patch, output, 809) == D5NEXT_OK);
        CHECK(memcmp(output, original, 809) == 0);
    }
    Fixture(original, 2);
    patch = Patch(D5NEXT_PATCH_EFFECT_FLAG, 2);
    patch.mask = 2;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    Fixture(original, 1);
    patch = Patch(D5NEXT_PATCH_COLOR, 1);
    patch.mask = 3;
    patch.hue = 1535;
    patch.saturation = 150;
    CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
    CHECK(output[706] == 5 && output[707] == 255 && output[708] == 150 && output[709] == 11);
    patch.hue = 1536;
    Reject(original, 809, identity, patch, D5NEXT_INVALID_LIGHTING);
    patch.hue = 0;
    patch.mask = 7;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    patch.mask = 3;
    patch.index = 1;
    Reject(original, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    patch.index = 0;
    original[662] = 7;
    FixCRC(original);
    Reject(original, 809, identity, patch, D5NEXT_PRECONDITION_FAILED);
}

static void TestValidation()
{
    unsigned char report[809];
    Fixture(report);
    D5NextRGBpxPatch patch = Patch(D5NEXT_PATCH_MASTER, 0x13);
    patch.value = 255;
    D5NextRGBpxConfiguration output = {};
    output.master = 91;
    for(unsigned int firmware = 0; firmware <= 65535; firmware++)
    {
        if(firmware == 1027) continue;
        D5NextIdentity unknown = identity;
        unknown.firmware = firmware;
        CHECK(D5NextRGBpx::Decode(report, 809, unknown, output) == D5NEXT_UNSUPPORTED_FIRMWARE);
        CHECK(output.master == 91);
    }
    D5NextIdentity bad = identity;
    bad.product = 0xF012;
    Reject(report, 809, bad, patch, D5NEXT_INVALID_ARGUMENT);
    bad = identity;
    bad.vendor = 0;
    Reject(report, 809, bad, patch, D5NEXT_INVALID_ARGUMENT);
    bad = identity;
    bad.interface_number = 0;
    Reject(report, 809, bad, patch, D5NEXT_INVALID_ARGUMENT);
    for(unsigned int size = 0; size < 809; size++)
    {
        CHECK(D5NextRGBpx::Decode(report, size, identity, output) == D5NEXT_INVALID_SIZE && output.master == 91);
    }
    CHECK(D5NextRGBpx::Decode(report, 810, identity, output) == D5NEXT_INVALID_SIZE);
    Reject(nullptr, 809, identity, patch, D5NEXT_INVALID_ARGUMENT);
    report[0] = 2;
    Reject(report, 809, identity, patch, D5NEXT_INVALID_REPORT_ID);
    report[0] = 3;
    report[100] ^= 1;
    Reject(report, 809, identity, patch, D5NEXT_INVALID_CRC);
    report[100] ^= 1;
    report[2] = 4;
    FixCRC(report);
    Reject(report, 809, identity, patch, D5NEXT_UNSUPPORTED_STRUCTURE);
    CHECK(D5NextRGBpx::Decode(report, 809, identity, output) == D5NEXT_UNSUPPORTED_STRUCTURE && output.master == 91);
    Fixture(report, 0x14);
    patch.expected_mode = 0x14;
    Reject(report, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    Fixture(report, 6);
    patch.expected_mode = 6;
    Reject(report, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
    Fixture(report, 3);
    patch = Patch(D5NEXT_PATCH_PARAMETER, 3);
    patch.value = 51;
    Reject(report, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION); /* Speed only compatible. */
    unsigned char save[11];
    const unsigned char expected[] = { 2, 0, 0, 0, 2, 0, 0, 0, 0, 0x34, 0xC6 };
    CHECK(D5NextSystemCommands::EncodeSettingsSave(save, 11) == D5NEXT_OK);
    CHECK(memcmp(save, expected, 11) == 0 && CRC(save + 1, 8) == 0x34C6);
    memset(save, 0xA5, 11);
    CHECK(D5NextSystemCommands::EncodeSettingsSave(save, 10) == D5NEXT_INVALID_SIZE);
    CHECK(D5NextSystemCommands::EncodeSettingsSave(nullptr, 11) == D5NEXT_INVALID_ARGUMENT);
    for(unsigned int index = 0; index < 11; index++) CHECK(save[index] == 0xA5);
}

/* Read the independently maintained lot-5 CSV, not a generated test mirror. */
static std::vector<std::string> CSVRow(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for(std::size_t index = 0; index < line.size(); index++)
    {
        char ch = line[index];
        if(ch == '"')
        {
            if(quoted && index + 1 < line.size() && line[index + 1] == '"')
            {
                field += '"';
                index++;
            }
            else quoted = !quoted;
        }
        else if(ch == ',' && !quoted)
        {
            fields.push_back(field);
            field.clear();
        }
        else if(ch != '\r') field += ch;
    }
    CHECK(!quoted);
    fields.push_back(field);
    return fields;
}

static void TestCatalogue(const char* csv_path)
{
    CHECK(D5NextEffects::Count() == 24 && D5NextEffects::Find(6) == nullptr);
    CHECK(D5NextEffects::At(24) == nullptr && D5NextEffects::Find(256) == nullptr);
    unsigned int parameters = 0, flags = 0, captured = 0, pc = 0;
    bool ids[256] = {};
    for(unsigned int index = 0; index < D5NextEffects::Count(); index++)
    {
        const D5NextEffectInfo* effect = D5NextEffects::At(index);
        CHECK(effect != nullptr && !ids[effect->id]);
        CHECK(!effect->production_editable);
        ids[effect->id] = true;
        parameters += effect->parameter_count;
        flags += effect->flag_count;
        captured += effect->captured;
        pc += effect->requires_pc_transport;
        for(unsigned int item = 0; item < effect->parameter_count; item++)
        {
            CHECK(!effect->parameters[item].range_known);
        }
    }
    CHECK(parameters == 64 && flags == 43 && captured == 23 && pc == 5);
    CHECK(strcmp(D5NextEffects::Parameter(0x0E, 0)->name, "Intensity") == 0);
    CHECK(strcmp(D5NextEffects::Parameter(0x0B, 4)->name, "Color change speed") == 0);
    CHECK(D5NextEffects::Parameter(0x0B, 3) == nullptr);
    CHECK(strcmp(D5NextEffects::Flag(3, 2)->name, "Reverse direction") == 0);
    CHECK(strcmp(D5NextEffects::Flag(4, 2)->name, "Fade in") == 0);
    CHECK(D5NextEffects::Flag(3, 4) == nullptr);
    CHECK(D5NextEffects::Parameter(3, 0)->evidence == D5NEXT_COMPATIBLE);

    std::ifstream input(csv_path);
    CHECK(input.good());
    std::string line;
    CHECK((bool)std::getline(input, line));
    unsigned int count = 0;
    while(std::getline(input, line))
    {
        std::vector<std::string> row = CSVRow(line);
        CHECK(row.size() == 11);
        unsigned int mode = (unsigned int)std::stoul(row[0], nullptr, 16);
        const D5NextEffectInfo* effect = D5NextEffects::Find(mode);
        CHECK(effect != nullptr && effect->name == row[1]);
        if(row[3].find("g6.values[") == 0)
        {
            unsigned int index = (unsigned int)std::stoul(row[3].substr(10));
            const D5NextParameterInfo* parameter = D5NextEffects::Parameter(mode, index);
            CHECK(parameter != nullptr && parameter->name == row[2]);
            CHECK(parameter->evidence == (row[8] == "confirme_isole" ? D5NEXT_ISOLATED : D5NEXT_COMPATIBLE));
            for(unsigned int column = 5; column <= 6; column++)
            {
                if(row[column] == "non_capture") continue;
                int value = std::stoi(row[column]);
                bool found = false;
                for(unsigned int sample = 0; sample < parameter->observed_count; sample++)
                {
                    found |= value == parameter->observed_values[sample];
                }
                CHECK(found);
            }
            if(parameter->evidence == D5NEXT_ISOLATED)
            {
                unsigned char original[809], output[809];
                Fixture(original, mode);
                int before = std::stoi(row[5]);
                unsigned int offset = 682 + 2 * index;
                original[offset] = (unsigned char)((unsigned int)before >> 8);
                original[offset + 1] = (unsigned char)before;
                FixCRC(original);
                D5NextRGBpxPatch patch = Patch(D5NEXT_PATCH_PARAMETER, mode);
                patch.index = index;
                patch.value = std::stoi(row[6]);
                CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
                CHECK(output[offset] == ((unsigned int)patch.value >> 8) && output[offset + 1] == (patch.value & 255));
                for(unsigned int byte = 0; byte < 807; byte++)
                {
                    CHECK(byte == offset || byte == offset + 1 || output[byte] == original[byte]);
                }
            }
        }
        else if(!row[4].empty())
        {
            const D5NextFlagInfo* flag = D5NextEffects::Flag(mode, (unsigned int)std::stoul(row[4], nullptr, 16));
            CHECK(flag != nullptr && flag->name == row[2]);
            unsigned char original[809], output[809];
            Fixture(original, mode);
            unsigned int before = 0xA501 | (unsigned int)std::stoul(row[5], nullptr, 16);
            unsigned int after = (unsigned int)std::stoul(row[6], nullptr, 16);
            original[664] = (unsigned char)(before >> 8);
            original[665] = (unsigned char)before;
            FixCRC(original);
            D5NextRGBpxPatch patch = Patch(D5NEXT_PATCH_EFFECT_FLAG, mode);
            patch.mask = flag->mask;
            patch.enabled = (after & flag->mask) != 0;
            CHECK(D5NextRGBpx::BuildPatch(original, 809, identity, patch, output, 809) == D5NEXT_OK);
            unsigned int expected = (before & ~flag->mask) | (after & flag->mask);
            CHECK((((unsigned int)output[664] << 8) | output[665]) == expected);
            for(unsigned int byte = 0; byte < 807; byte++)
            {
                CHECK(byte == 664 || byte == 665 || output[byte] == original[byte]);
            }
        }
        else
        {
            CHECK(mode == 4 && row[2] == "Background");
            CHECK(D5NextEffects::Color(mode, 0)->evidence == D5NEXT_ISOLATED);
        }
        count++;
    }
    CHECK(count == 66);
}


/* No fixtures, CSV or report parsing: interrogate the linked production table
 * and require lookup functions to return its actual member addresses. */
static void TestLinkedCatalogueIntegrity()
{
    const struct
    {
        unsigned int id;
        const char* name;
        unsigned int parameter_indices;
        unsigned int flag_bits;
        D5NextHostEvidence host;
    } expected[] = {
        {0x0D,"Bar1",0,0,D5NEXT_HOST_UNKNOWN},
        {0x0F,"Rain1",0x00F,0x1A,D5NEXT_HOST_UNKNOWN},
        {0x10,"Rain2",0x00F,0x1A,D5NEXT_HOST_UNKNOWN},
        {0x11,"Rain3",0x00F,0x1A,D5NEXT_HOST_UNKNOWN},
        {0x12,"Switch",0,0,D5NEXT_HOST_UNKNOWN},
        {0x14,"Sound1",0,0,D5NEXT_HOST_PROBABLE},
        {0x15,"Sound2",0x0BB,0x2F,D5NEXT_HOST_PROBABLE},
        {0x16,"Sound3",0x1FF,0,D5NEXT_HOST_PROBABLE},
        {0x17,"Sound4",0x01D,0x07,D5NEXT_HOST_PROBABLE},
        {0x18,"AmbientPx",0,0,D5NEXT_HOST_OBSERVED},
    };
    unsigned int parameters = 0, isolated = 0, coupled = 0, flags = 0, filters = 0, colors = 0;
    for(const auto& item : expected)
    {
        const auto* effect = D5NextEffects::Find(item.id);
        CHECK(effect && effect->id == item.id && strcmp(effect->name,item.name) == 0);
        CHECK(effect->captured && !effect->production_editable && effect->host_dependency == item.host);
        unsigned int enumerated = 0;
        for(unsigned int i = 0; i < D5NextEffects::Count(); i++)
            enumerated += D5NextEffects::At(i) == effect;
        CHECK(enumerated == 1);
        unsigned int seen_indices = 0, seen_flags = 0;
        CHECK((effect->parameters != nullptr) == (effect->parameter_count != 0));
        CHECK((effect->flags != nullptr) == (effect->flag_count != 0));
        for(unsigned int i = 0; i < effect->parameter_count; i++)
        {
            const auto& parameter = effect->parameters[i];
            CHECK(parameter.index < 12 && !(seen_indices & (1U << parameter.index)));
            seen_indices |= 1U << parameter.index;
            CHECK(D5NextEffects::Parameter(item.id,parameter.index) == &parameter);
            CHECK(parameter.name && parameter.frame && parameter.observed_values && parameter.observed_count);
            CHECK(!parameter.range_known);
            const bool is_coupled = item.id == 0x15
                && (parameter.index == 1 || parameter.index == 3 || parameter.index == 5);
            CHECK(parameter.evidence == (is_coupled ? D5NEXT_COMPATIBLE : D5NEXT_ISOLATED));
            parameters++;
            is_coupled ? coupled++ : isolated++;
        }
        CHECK(seen_indices == item.parameter_indices);
        for(unsigned int index = 0; index <= 12; index++)
            CHECK((D5NextEffects::Parameter(item.id,index) != nullptr)
                  == ((item.parameter_indices & (1U << index)) != 0));
        for(unsigned int i = 0; i < effect->flag_count; i++)
        {
            const auto& flag = effect->flags[i];
            CHECK(flag.mask && !(seen_flags & flag.mask));
            seen_flags |= flag.mask;
            CHECK(D5NextEffects::Flag(item.id,flag.mask) == &flag);
            CHECK(flag.name && flag.frame && flag.evidence == D5NEXT_ISOLATED);
            flags++;
        }
        CHECK(seen_flags == item.flag_bits);
        for(unsigned int mask = 1; mask <= 0x8000; mask <<= 1)
            CHECK((D5NextEffects::Flag(item.id,mask) != nullptr) == ((item.flag_bits & mask) != 0));
        CHECK(D5NextEffects::Flag(item.id,0) == nullptr);
        CHECK(effect->source_filter_count == (item.id == 0x15 ? 2U : 0U));
        CHECK(effect->color_count == (item.id == 0x15 ? 2U : 0U));
        for(unsigned int i = 0; i < effect->source_filter_count; i++)
        {
            const auto& filter = effect->source_filters[i];
            CHECK(filter.index == i && filter.evidence == D5NEXT_ISOLATED && filter.observed_count == 2);
            CHECK(filter.observed_values[0] == 1 && filter.observed_values[1] == (i == 0 ? 38 : 58));
            CHECK(strcmp(filter.name,i == 0 ? "Rising values" : "Falling values") == 0);
            filters++;
        }
        for(unsigned int i = 0; i < effect->color_count; i++)
        {
            const auto& color = effect->colors[i];
            CHECK(D5NextEffects::Color(item.id,color.index) == &color);
            CHECK(color.evidence == D5NEXT_ISOLATED && color.components == (D5NEXT_COLOR_H | D5NEXT_COLOR_S));
            colors++;
        }
    }
    CHECK(parameters == 31 && isolated == 28 && coupled == 3 && flags == 17 && filters == 2 && colors == 2);
    CHECK(D5NextEffects::Find(6) == nullptr && D5NextEffects::Find(0)->captured == false);
    std::printf("PASS linked C++ catalogue integrity: %u parameters (%u isolated/%u coupled), %u flags, %u filters, %u colors\n",
                parameters,isolated,coupled,flags,filters,colors);
}

static void TestAdvancedCatalogue(const char* path)
{
    std::ifstream input(path);
    CHECK(input.good());
    std::string line;
    CHECK((bool)std::getline(input, line));
    unsigned int isolated = 0, coupled = 0, numeric = 0, flags = 0, filters = 0, colors = 0;
    while(std::getline(input, line))
    {
        auto row = CSVRow(line);
        CHECK(row.size() == 12);
        unsigned int mode = std::stoul(row[0], nullptr, 16);
        const auto* effect = D5NextEffects::Find(mode);
        CHECK(effect && effect->captured && !effect->production_editable);
        bool confirmed = row[9].find("confirme") == 0;
        confirmed ? isolated++ : coupled++;
        D5NextEvidence evidence = confirmed ? D5NEXT_ISOLATED : D5NEXT_COMPATIBLE;
        unsigned int index = row[3].find('[') == std::string::npos ? 0
            : std::stoul(row[3].substr(row[3].find('[') + 1));
        if(row[3].find("values[") != std::string::npos)
        {
            const auto* parameter = D5NextEffects::Parameter(mode, index);
            CHECK(parameter && parameter->name == row[2] && parameter->evidence == evidence);
            CHECK(!parameter->range_known && parameter->observed_count > 0);
            CHECK(parameter->frame == std::stoul(row[7]));
            if(confirmed) numeric++;
        }
        else if(!row[5].empty())
        {
            const auto* flag = D5NextEffects::Flag(mode, std::stoul(row[5], nullptr, 16));
            CHECK(flag && flag->name == row[2] && flag->evidence == evidence);
            CHECK(flag->frame == std::stoul(row[7]));
            flags++;
        }
        else if(row[3].find("filter[") != std::string::npos)
        {
            bool found = false;
            for(unsigned int i = 0; i < effect->source_filter_count; i++)
            {
                const auto& filter = effect->source_filters[i];
                if(filter.index == index)
                {
                    CHECK(filter.name == row[2] && filter.evidence == evidence);
                    CHECK(filter.frame == std::stoul(row[7]) && filter.observed_count == 2);
                    found = true;
                }
            }
            CHECK(found);
            filters++;
        }
        else
        {
            const auto* color = D5NextEffects::Color(mode, index);
            CHECK(color && color->role == row[2] && color->evidence == evidence);
            CHECK(color->components == (D5NEXT_COLOR_H | D5NEXT_COLOR_S));
            colors++;
        }
        unsigned char report[809], output[809];
        Fixture(report, mode);
        D5NextRGBpxPatch patch = Patch(D5NEXT_PATCH_PARAMETER, mode);
        patch.index = index;
        patch.value = 0;
        memset(output, 0xA5, sizeof(output));
        CHECK(D5NextRGBpx::BuildPatch(report, 809, identity, patch, output, 809) == D5NEXT_UNSUPPORTED_OPERATION);
        CHECK(D5NextRGBpx::AuditPatch(report, 809, identity, patch, output, 809) == D5NEXT_UNSUPPORTED_OPERATION);
        for(unsigned char byte : output) CHECK(byte == 0xA5);
    }
    CHECK(isolated == 49 && coupled == 3 && numeric == 28 && flags == 17 && filters == 2 && colors == 2);
    const unsigned int advanced[] = { 0x0D,0x0F,0x10,0x11,0x12,0x14,0x15,0x16,0x17,0x18 };
    for(unsigned int mode : advanced)
    {
        const auto* effect = D5NextEffects::Find(mode);
        CHECK(effect && effect->captured && !effect->production_editable);
        CHECK(effect->host_dependency == (mode == 0x18 ? D5NEXT_HOST_OBSERVED
              : mode >= 0x14 ? D5NEXT_HOST_PROBABLE : D5NEXT_HOST_UNKNOWN));
        unsigned char report[809];
        Fixture(report, mode);
        for(unsigned int kind = D5NEXT_PATCH_MASTER; kind <= D5NEXT_PATCH_MODE; kind++)
        {
            auto patch = Patch((D5NextPatchKind)kind, mode);
            Reject(report, 809, identity, patch, D5NEXT_UNSUPPORTED_OPERATION);
        }
    }
    CHECK(D5NextEffects::Parameter(0x0D,0) == nullptr);
    CHECK(D5NextEffects::Parameter(0x12,0) == nullptr);
    CHECK(D5NextEffects::Parameter(0x14,0) == nullptr);
    CHECK(D5NextEffects::Parameter(0x18,0) == nullptr);
    CHECK(D5NextEffects::Parameter(0x17,1) == nullptr); // Not an isolated CSV attribution.
    for(unsigned int index : {1U,3U,5U})
        CHECK(D5NextEffects::Parameter(0x15,index)->evidence == D5NEXT_COMPATIBLE);
}

static void TestAdvancedLiteralVectors()
{
    // Independent documentary values and literal byte offsets, not generated from catalogue.
    const struct { unsigned int mode,index,offset,before,after; } vectors[] = {
        {15,0,682,40,20},{15,1,684,4,3},{15,2,686,25,57},{15,3,688,30,63},
        {16,0,682,50,64},{16,1,684,3,2},{16,2,686,10,38},{16,3,688,15,50},
        {17,0,682,25,61},{17,1,684,3,2},{17,2,686,60,28},{17,3,688,15,64},
        {21,0,682,0,2},{21,4,690,25,55},{21,7,696,5,10},
        {22,0,682,0,1},{22,1,684,0,2},{22,2,686,0,3},{22,3,688,0,4},
        {22,4,690,5,3},{22,5,692,5,3},{22,6,694,5,3},{22,7,696,5,3},{22,8,698,10,58},
        {23,0,682,10,43},{23,2,686,2,6},{23,3,688,10,43},{23,4,690,50,25}
    };
    CHECK(sizeof(vectors)/sizeof(vectors[0]) == 28);
    for(const auto& vector : vectors)
    {
        const auto* parameter = D5NextEffects::Parameter(vector.mode, vector.index);
        CHECK(parameter && parameter->evidence == D5NEXT_ISOLATED);
        for(unsigned int value : {vector.before, vector.after})
        {
            bool sample = false;
            for(unsigned int i = 0; i < parameter->observed_count; i++) sample |= value == (unsigned int)parameter->observed_values[i];
            CHECK(sample);
            unsigned char report[809], snapshot[809], rebuilt[809], group[70];
            Fixture(report, vector.mode);
            report[vector.offset] = 0; // All these documentary samples are <= 100.
            report[vector.offset+1] = (unsigned char)value;
            FixCRC(report); // Independent bitwise CRC oracle.
            memcpy(snapshot, report, 809);
            D5NextRGBpxConfiguration decoded;
            CHECK(D5NextRGBpx::Decode(report,809,identity,decoded) == D5NEXT_OK);
            CHECK(decoded.groups[6].values[vector.index] == (int)value);
            CHECK(D5NextRGBpx::EncodeController(decoded.groups[6],group,70) == D5NEXT_OK);
            CHECK(memcmp(group,report+660,70) == 0);
            memcpy(rebuilt,report,809);
            memcpy(rebuilt+660,group,70);
            unsigned short crc = D5NextProtocol::CalculateCRC(rebuilt+1,806);
            rebuilt[807] = (unsigned char)(crc>>8); rebuilt[808] = (unsigned char)crc;
            CHECK(memcmp(snapshot,rebuilt,809) == 0 && memcmp(snapshot,report,809) == 0);
        }
    }
    const struct { unsigned int mode,mask; const char* name; } flags[] = {
        {15,2,"Reverse direction"},{15,8,"Random color"},{15,16,"Snow"},
        {16,2,"Reverse direction"},{16,8,"Random color"},{16,16,"Snow"},
        {17,2,"Reverse direction"},{17,8,"Random color"},{17,16,"Snow"},
        {21,8,"Reverse direction"},{21,2,"Show bar"},{21,4,"Show ranges"},
        {21,1,"Fade ranges"},{21,32,"Show peak value"},
        {23,2,"Effect 1: Random color"},{23,4,"Effect 2: Random color"},{23,1,"Reverse direction"}
    };
    for(const auto& flag : flags)
    {
        const auto* metadata = D5NextEffects::Flag(flag.mode,flag.mask);
        CHECK(metadata && strcmp(metadata->name,flag.name) == 0);
        unsigned char report[809]; Fixture(report,flag.mode);
        report[664]=0xA5; report[665]=(unsigned char)(0xC0|flag.mask); FixCRC(report);
        D5NextRGBpxConfiguration decoded;
        CHECK(D5NextRGBpx::Decode(report,809,identity,decoded) == D5NEXT_OK);
        CHECK(decoded.groups[6].flags == (0xA5C0|flag.mask));
        unsigned char encoded[70];
        CHECK(D5NextRGBpx::EncodeController(decoded.groups[6],encoded,70) == D5NEXT_OK);
        CHECK(memcmp(encoded,report+660,70) == 0);
    }
    // Independent offsets for the remaining four isolated attributions.
    unsigned char sound[809]; Fixture(sound,0x15);
    sound[668]=38;sound[669]=58;
    sound[710]=0x04;sound[711]=0xB9;sound[712]=240; // HSV1 peak, H1209/S240.
    sound[714]=0x04;sound[715]=0xF7;sound[716]=133; // HSV2 range, H1271/S133.
    FixCRC(sound);
    D5NextRGBpxConfiguration sound_config;
    CHECK(D5NextRGBpx::Decode(sound,809,identity,sound_config) == D5NEXT_OK);
    CHECK(sound_config.groups[6].source_filter[0]==38 && sound_config.groups[6].source_filter[1]==58);
    CHECK(sound_config.groups[6].colors[1].hue==1209 && sound_config.groups[6].colors[1].saturation==240);
    CHECK(sound_config.groups[6].colors[2].hue==1271 && sound_config.groups[6].colors[2].saturation==133);
    CHECK(sound_config.groups[6].colors[1].value==12 && sound_config.groups[6].colors[2].value==13);
    unsigned char report[809]; Fixture(report,15);
    report[682]=0x80;report[683]=0;report[684]=0xFF;report[685]=0xFE;FixCRC(report);
    D5NextRGBpxConfiguration decoded;
    CHECK(D5NextRGBpx::Decode(report,809,identity,decoded) == D5NEXT_OK);
    CHECK(decoded.groups[6].values[0] == -32768 && decoded.groups[6].values[1] == -2);
    // Capture11 topology, synthetic fields only; active overlapping g7 is preserved.
    report[240]=0;report[241]=0;report[242]=13;report[243]=2;
    report[730]=1;report[731]=0;report[732]=6;report[733]=2;FixCRC(report);
    CHECK(D5NextRGBpx::Decode(report,809,identity,decoded) == D5NEXT_OK);
    CHECK(decoded.groups[0].count == 13 && decoded.groups[6].count == 6 && decoded.groups[7].mode == 2);
    for(unsigned int group : {0U,7U})
    {
        auto patch = Patch(D5NEXT_PATCH_MASTER,2);patch.group=group;
        Reject(report,809,identity,patch,D5NEXT_UNSUPPORTED_OPERATION);
    }
}

static void TestDocumentaryBulk()
{
    unsigned char audio[35] = {0x0A};
    unsigned short crc = CRC(audio+1,32);
    audio[33]=(unsigned char)(crc>>8);audio[34]=(unsigned char)crc;
    CHECK(DecodeObservedBulk35(audio,35));
    audio[0]=0x0B;CHECK(!DecodeObservedBulk35(audio,35));audio[0]=0x0A;
    for(unsigned int size=0;size<35;size++) CHECK(!DecodeObservedBulk35(audio,size));
    CHECK(!DecodeObservedBulk35(audio,36) && !DecodeObservedBulk35(nullptr,35));
    audio[1]=1;CHECK(!DecodeObservedBulk35(audio,35));audio[1]=0;
    audio[34]^=1;CHECK(!DecodeObservedBulk35(audio,35));
    unsigned char screen[364] = {0x0B,6,6,0,
        0,0,255,0, 0,255,0,0, 0,0,0,255, 0,255,255,0, 0,255,0,255, 0,0,255,255};
    DocumentaryColor colors[6] = {};
    CHECK(DecodeObservedBulk364(screen,364,colors));
    CHECK(colors[0].red==255 && colors[0].green==0 && colors[0].blue==0);
    CHECK(colors[1].green==255 && colors[2].blue==255);
    CHECK(colors[3].red==255 && colors[3].green==255 && colors[3].blue==0);
    DocumentaryColor snapshot[6];memcpy(snapshot,colors,sizeof(colors));
    for(unsigned int size=0;size<364;size++) CHECK(!DecodeObservedBulk364(screen,size,colors));
    CHECK(!DecodeObservedBulk364(screen,365,colors) && !DecodeObservedBulk364(nullptr,364,colors));
    for(unsigned int offset : {0U,1U,2U,3U,4U,28U,363U})
    {
        screen[offset]^=1;CHECK(!DecodeObservedBulk364(screen,364,colors));screen[offset]^=1;
        CHECK(memcmp(snapshot,colors,sizeof(colors))==0);
    }
}

int main(int argc, char** argv)
{
    if(argc == 2 && strcmp(argv[1],"--catalogue-integrity") == 0)
    {
        TestLinkedCatalogueIntegrity();
        std::printf("PASS catalogue-only checks (%u assertions); no CSV or fixture loaded\n",checks);
        return 0;
    }
    CHECK(argc == 3);
    TestLinkedCatalogueIntegrity();
    TestEndian();
    TestModel();
    TestLiteralControllerVector();
    TestPatches();
    TestValidation();
    TestCatalogue(argv[1]);
    TestAdvancedCatalogue(argv[2]);
    TestAdvancedLiteralVectors();
    TestDocumentaryBulk();
    std::printf("PASS RGBpx codec, signed fields, catalogue/CSV, exact patches, and offline Output02 (%u checks); no HID backend\n", checks);
    return 0;
}
