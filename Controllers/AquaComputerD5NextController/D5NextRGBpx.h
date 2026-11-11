/*---------------------------------------------------------*\
| D5NextRGBpx.h                                             |
|                                                           |
|   Transport-independent RGBpx model and minimal patches   |
|   Protocol evidence: D5NextRGBpxVideoUSBMapping.md         |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "D5NextProtocol.h"

#define D5NEXT_FIRMWARE              1027
#define D5NEXT_STRUCTURE             3
#define D5NEXT_GROUP_COUNT           8
#define D5NEXT_GROUP_SIZE            70
#define D5NEXT_GROUP_OFFSET          240
#define D5NEXT_INTERNAL_GROUP        6
#define D5NEXT_PARAMETER_COUNT       12
#define D5NEXT_COLOR_COUNT           6
#define D5NEXT_STRIP_OFF_MASK        0x0002
#define D5NEXT_COLOR_H               0x01
#define D5NEXT_COLOR_S               0x02
#define D5NEXT_COLOR_V               0x04

/* A supplied identity is evidence about the input, never write permission. */
struct D5NextIdentity
{
    unsigned int vendor;
    unsigned int product;
    int          interface_number;
    unsigned int firmware;
};

struct D5NextBinding
{
    short          x1;
    short          x2;
    unsigned char  y1;
    unsigned char  y2;
};

struct D5NextRGBpxController
{
    unsigned char  strip;
    unsigned char  led_start;
    unsigned char  count;
    unsigned char  mode;
    unsigned short flags;
    short          source_id;
    unsigned char  source_filter[2];
    D5NextBinding  binding[2];
    short          values[D5NEXT_PARAMETER_COUNT];
    D5NextHSV      colors[D5NEXT_COLOR_COUNT];
};

/* Only decoded RGB fields. Opaque/hydraulic bytes stay in the fresh buffer. */
struct D5NextRGBpxConfiguration
{
    unsigned short          structure;
    unsigned char           master;
    unsigned short          strip_flags;
    D5NextRGBpxController    groups[D5NEXT_GROUP_COUNT];
};

enum D5NextEvidence
{
    D5NEXT_DECLARED,
    D5NEXT_OBSERVED,
    D5NEXT_COMPATIBLE,
    D5NEXT_ISOLATED,
};

struct D5NextParameterInfo
{
    unsigned int        index;
    const char*         name;
    D5NextEvidence      evidence;
    const short*        observed_values;
    unsigned int        observed_count;
    bool                range_known;
    short               minimum;
    short               maximum;
    unsigned int        frame;
};

struct D5NextFlagInfo
{
    unsigned short      mask;
    const char*         name;
    unsigned int        frame;
    D5NextEvidence      evidence = D5NEXT_ISOLATED;
};

struct D5NextColorInfo
{
    unsigned int        index;
    const char*         role;
    unsigned int        components;
    D5NextEvidence      evidence;
};

enum D5NextHostEvidence
{
    D5NEXT_HOST_UNKNOWN,
    D5NEXT_HOST_PROBABLE,
    D5NEXT_HOST_OBSERVED,
};

struct D5NextFilterInfo
{
    unsigned int index;
    const char* name;
    D5NextEvidence evidence;
    const short* observed_values;
    unsigned int observed_count;
    unsigned int frame;
};

struct D5NextEffectInfo
{
    unsigned char               id;
    const char*                 name;
    bool                        captured; /* Mode selected, not an edit approval. */
    bool                        requires_pc_transport; /* Legacy conservative PC classification. */
    const D5NextParameterInfo*   parameters;
    unsigned int                parameter_count;
    const D5NextFlagInfo*        flags;
    unsigned int                flag_count;
    const D5NextColorInfo*       colors;
    unsigned int                color_count;
    D5NextHostEvidence          host_dependency = D5NEXT_HOST_UNKNOWN;
    /* Descriptive only. Hardware permission is controlled outside the catalogue. */
    bool                       production_editable = false;
    const D5NextFilterInfo*     source_filters = nullptr;
    unsigned int               source_filter_count = 0;
};

class D5NextEffects
{
public:
    static unsigned int                 Count();
    static const D5NextEffectInfo*       At(unsigned int index);
    static const D5NextEffectInfo*       Find(unsigned int id);
    static const D5NextParameterInfo*    Parameter(unsigned int mode, unsigned int index);
    static const D5NextFlagInfo*         Flag(unsigned int mode, unsigned int mask);
    static const D5NextColorInfo*        Color(unsigned int mode, unsigned int index);
};

enum D5NextPatchKind
{
    D5NEXT_PATCH_MASTER,
    D5NEXT_PATCH_STRIP_OFF,
    D5NEXT_PATCH_PARAMETER,
    D5NEXT_PATCH_EFFECT_FLAG,
    D5NEXT_PATCH_COLOR,
    D5NEXT_PATCH_MODE,
};

/* One action per patch. Wide request fields allow rejection before narrowing. */
struct D5NextRGBpxPatch
{
    D5NextPatchKind  kind;
    unsigned int    group;
    unsigned int    expected_mode;
    unsigned int    index;
    int             value;
    unsigned int    mask;
    bool            enabled;
    unsigned int    hue;
    unsigned int    saturation;
    unsigned int    hsv_value;
};

class D5NextRGBpx
{
public:
    static D5NextResult ValidateIdentity(const D5NextIdentity& identity);
    static bool ReadBE16(const unsigned char* bytes, std::size_t size, std::size_t offset, unsigned short& value);
    static bool ReadInt16(const unsigned char* bytes, std::size_t size, std::size_t offset, short& value);
    static bool WriteBE16(unsigned char* bytes, std::size_t size, std::size_t offset, unsigned int value);
    static bool WriteInt16(unsigned char* bytes, std::size_t size, std::size_t offset, int value);
    static D5NextHSV RGBToHSV(unsigned char red, unsigned char green, unsigned char blue);

    /* Decode preserves unknown modes/flags/hues; it does not approve edits. */
    static D5NextResult DecodeController(const unsigned char* bytes, std::size_t size, D5NextRGBpxController& controller);
    /* A 70-byte codec, not a builder for whole device configurations. */
    static D5NextResult EncodeController(const D5NextRGBpxController& controller, unsigned char* bytes, std::size_t size);
    static D5NextResult Decode(const unsigned char* report, std::size_t size, const D5NextIdentity& identity,
                               D5NextRGBpxConfiguration& configuration);
    /* Fresh input required by caller. No transport, allocation or saved presets.
     * Only internal group 6 may be edited; mode transitions refuse for now.
     * Numeric edits accept isolated observed samples, not invented UI ranges.
     * On error output is untouched, including overlapping input/output buffers. */
    static D5NextResult BuildPatch(const unsigned char* original, std::size_t size, const D5NextIdentity& identity,
                                   const D5NextRGBpxPatch& patch, unsigned char* output, std::size_t output_size);
    static D5NextResult AuditPatch(const unsigned char* original, std::size_t size, const D5NextIdentity& identity,
                                   const D5NextRGBpxPatch& patch, const unsigned char* candidate, std::size_t candidate_size);
};
