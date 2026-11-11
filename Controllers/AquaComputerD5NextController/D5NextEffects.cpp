/*---------------------------------------------------------*\
| D5NextEffects.cpp                                         |
|                                                           |
|   Descriptive catalogue from documented protocol facts    |
|   Lot 5 CSV: isolated edits, not firmware presets/ranges   |
|                                                           |
|   Yannick Peuchot ( Pepeuch )                        10 Nov 2026 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "D5NextRGBpx.h"

static const short Breathing_v0[] = { 60, 49 };
static const short Breathing_v1[] = { 80, 71 };
static const short Breathing_v2[] = { 5, 22 };
static const short Breathing_v3[] = { 20, 31 };
static const D5NextParameterInfo Breathing_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Breathing_v0, sizeof(Breathing_v0) / sizeof(short), false, 0, 0, 8805 },
    { 1, "Intensity", D5NEXT_ISOLATED, Breathing_v1, sizeof(Breathing_v1) / sizeof(short), false, 0, 0, 9039 },
    { 2, "Delay max. brightness", D5NEXT_ISOLATED, Breathing_v2, sizeof(Breathing_v2) / sizeof(short), false, 0, 0, 9307 },
    { 3, "Delay min. brightness", D5NEXT_ISOLATED, Breathing_v3, sizeof(Breathing_v3) / sizeof(short), false, 0, 0, 9511 },
};

static const short Rainbow_v0[] = { 51 };
static const short Rainbow_v1[] = { 100, 99 };
static const D5NextParameterInfo Rainbow_parameters[] =
{
    { 0, "Speed", D5NEXT_COMPATIBLE, Rainbow_v0, sizeof(Rainbow_v0) / sizeof(short), false, 0, 0, 3541 },
    { 1, "Color range", D5NEXT_ISOLATED, Rainbow_v1, sizeof(Rainbow_v1) / sizeof(short), false, 0, 0, 3795 },
};
static const D5NextFlagInfo Rainbow_flags[] =
{
    { 0x0002, "Reverse direction", 3999 },
};

static const short Blink1_v0[] = { 40, 61 };
static const D5NextParameterInfo Blink1_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Blink1_v0, sizeof(Blink1_v0) / sizeof(short), false, 0, 0, 14213 },
};
static const D5NextFlagInfo Blink1_flags[] =
{
    { 0x0002, "Fade in", 14253 },
    { 0x0004, "Fade out", 14341 },
    { 0x0008, "Random color", 14411 },
    { 0x0010, "Slide colors", 14493 },
};

static const short Blink2_v0[] = { 40, 56 };
static const D5NextParameterInfo Blink2_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Blink2_v0, sizeof(Blink2_v0) / sizeof(short), false, 0, 0, 11699 },
};
static const D5NextFlagInfo Blink2_flags[] =
{
    { 0x0008, "Random color", 11939 },
    { 0x0010, "Slide colors", 12015 },
    { 0x0004, "Fade", 12123 },
};

static const short Sequential_v0[] = { 25, 56 };
static const short Sequential_v1[] = { 30, 17 };
static const short Sequential_v3[] = { 10, 43 };
static const short Sequential_v4[] = { 5, 27 };
static const D5NextParameterInfo Sequential_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Sequential_v0, sizeof(Sequential_v0) / sizeof(short), false, 0, 0, 17933 },
    { 1, "Smoothness", D5NEXT_ISOLATED, Sequential_v1, sizeof(Sequential_v1) / sizeof(short), false, 0, 0, 18201 },
    { 3, "Delay after sequence", D5NEXT_ISOLATED, Sequential_v3, sizeof(Sequential_v3) / sizeof(short), false, 0, 0, 18449 },
    { 4, "Delay before sequence", D5NEXT_ISOLATED, Sequential_v4, sizeof(Sequential_v4) / sizeof(short), false, 0, 0, 18823 },
};
static const D5NextFlagInfo Sequential_flags[] =
{
    { 0x0008, "Random color", 19051 },
    { 0x0002, "Reverse direction", 19087 },
    { 0x0004, "Fade", 19311 },
};

static const short Visor1_v0[] = { 25, 48 };
static const short Visor1_v1[] = { 40, 53 };
static const short Visor1_v2[] = { 20, 41 };
static const D5NextParameterInfo Visor1_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Visor1_v0, sizeof(Visor1_v0) / sizeof(short), false, 0, 0, 20441 },
    { 1, "Smoothness", D5NEXT_ISOLATED, Visor1_v1, sizeof(Visor1_v1) / sizeof(short), false, 0, 0, 20583 },
    { 2, "Width", D5NEXT_ISOLATED, Visor1_v2, sizeof(Visor1_v2) / sizeof(short), false, 0, 0, 20865 },
};
static const D5NextFlagInfo Visor1_flags[] =
{
    { 0x0040, "Color change", 20977 },
    { 0x0008, "Random color", 21017 },
    { 0x0002, "Reverse direction", 21203 },
    { 0x0020, "Color mode 2", 21217 },
    { 0x0080, "Circular", 21315 },
};

static const short Visor2_v0[] = { 25, 41 };
static const short Visor2_v1[] = { 40, 29 };
static const short Visor2_v2[] = { 20, 30 };
static const D5NextParameterInfo Visor2_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Visor2_v0, sizeof(Visor2_v0) / sizeof(short), false, 0, 0, 22273 },
    { 1, "Smoothness", D5NEXT_ISOLATED, Visor2_v1, sizeof(Visor2_v1) / sizeof(short), false, 0, 0, 22449 },
    { 2, "Width", D5NEXT_ISOLATED, Visor2_v2, sizeof(Visor2_v2) / sizeof(short), false, 0, 0, 22639 },
};
static const D5NextFlagInfo Visor2_flags[] =
{
    { 0x0040, "Color change", 23025 },
    { 0x0008, "Random color", 23055 },
    { 0x0002, "Reverse direction", 23413 },
    { 0x0020, "Color mode 2", 23445 },
    { 0x0080, "Circular", 23569 },
};

static const short Marquee1_v0[] = { 25, 53 };
static const short Marquee1_v1[] = { 40, 26 };
static const short Marquee1_v2[] = { 20, 47 };
static const D5NextParameterInfo Marquee1_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Marquee1_v0, sizeof(Marquee1_v0) / sizeof(short), false, 0, 0, 24617 },
    { 1, "Smoothness", D5NEXT_ISOLATED, Marquee1_v1, sizeof(Marquee1_v1) / sizeof(short), false, 0, 0, 24867 },
    { 2, "Width", D5NEXT_ISOLATED, Marquee1_v2, sizeof(Marquee1_v2) / sizeof(short), false, 0, 0, 25123 },
};
static const D5NextFlagInfo Marquee1_flags[] =
{
    { 0x0002, "Reverse direction", 25517 },
    { 0x0008, "Random color", 25597 },
    { 0x0080, "Circular", 25681 },
};

static const short Marquee2_v0[] = { 30, 31 };
static const short Marquee2_v1[] = { 40, 50 };
static const short Marquee2_v4[] = { 80, 39 };
static const D5NextParameterInfo Marquee2_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Marquee2_v0, sizeof(Marquee2_v0) / sizeof(short), false, 0, 0, 15877 },
    { 1, "Smoothness", D5NEXT_ISOLATED, Marquee2_v1, sizeof(Marquee2_v1) / sizeof(short), false, 0, 0, 16193 },
    { 4, "Color change speed", D5NEXT_ISOLATED, Marquee2_v4, sizeof(Marquee2_v4) / sizeof(short), false, 0, 0, 16409 },
};
static const D5NextFlagInfo Marquee2_flags[] =
{
    { 0x0008, "Random color", 16647 },
    { 0x0002, "Reverse direction", 16771 },
};

static const short Ambient_v0[] = { 30, 42 };
static const short Ambient_v1[] = { 50, 38 };
static const short Ambient_v2[] = { 25, 36 };
static const D5NextParameterInfo Ambient_parameters[] =
{
    { 0, "Speed", D5NEXT_ISOLATED, Ambient_v0, sizeof(Ambient_v0) / sizeof(short), false, 0, 0, 10243 },
    { 1, "Color range", D5NEXT_ISOLATED, Ambient_v1, sizeof(Ambient_v1) / sizeof(short), false, 0, 0, 10461 },
    { 2, "Total area", D5NEXT_ISOLATED, Ambient_v2, sizeof(Ambient_v2) / sizeof(short), false, 0, 0, 10613 },
};

static const short Candle_v0[] = { 50, 40, 67 };
static const D5NextParameterInfo Candle_parameters[] =
{
    { 0, "Intensity", D5NEXT_ISOLATED, Candle_v0, sizeof(Candle_v0) / sizeof(short), false, 0, 0, 26749 },
};

static const short RainbowShift_v0[] = { 35, 44 };
static const short RainbowShift_v1[] = { 10, 22 };
static const short RainbowShift_v2[] = { 5, 14 };
static const short RainbowShift_v3[] = { 90, 79 };
static const short RainbowShift_v4[] = { 25, 38 };
static const D5NextParameterInfo RainbowShift_parameters[] =
{
    { 0, "Point speed", D5NEXT_ISOLATED, RainbowShift_v0, sizeof(RainbowShift_v0) / sizeof(short), false, 0, 0, 5423 },
    { 1, "Point smoothness", D5NEXT_ISOLATED, RainbowShift_v1, sizeof(RainbowShift_v1) / sizeof(short), false, 0, 0, 5691 },
    { 2, "Point size", D5NEXT_ISOLATED, RainbowShift_v2, sizeof(RainbowShift_v2) / sizeof(short), false, 0, 0, 5907 },
    { 3, "Color change speed", D5NEXT_ISOLATED, RainbowShift_v3, sizeof(RainbowShift_v3) / sizeof(short), false, 0, 0, 6241 },
    { 4, "Color range", D5NEXT_ISOLATED, RainbowShift_v4, sizeof(RainbowShift_v4) / sizeof(short), false, 0, 0, 6423 },
};

/* Only these palette roles are established. V is decoded, not yet an
 * independently validated edit. Other palette counts/roles remain unknown. */
static const D5NextColorInfo FixedColor_colors[] =
{
    { 0, "Color", D5NEXT_COLOR_H | D5NEXT_COLOR_S, D5NEXT_OBSERVED },
};
static const D5NextColorInfo Blink1_colors[] =
{
    { 0, "Background", D5NEXT_COLOR_H | D5NEXT_COLOR_S, D5NEXT_ISOLATED },
};

/* Capture06 facts. Samples are not ranges or hardware write approval. */
static const short Rain1_parameters_0[] = { 40, 20 };
static const short Rain1_parameters_1[] = { 4, 3 };
static const short Rain1_parameters_2[] = { 25, 57 };
static const short Rain1_parameters_3[] = { 30, 63 };
static const D5NextParameterInfo Rain1_parameters[] =
{
    { 0, "Drop speed", D5NEXT_ISOLATED, Rain1_parameters_0, sizeof(Rain1_parameters_0) / sizeof(short), false, 0, 0, 1225 },
    { 1, "Drop items", D5NEXT_ISOLATED, Rain1_parameters_1, sizeof(Rain1_parameters_1) / sizeof(short), false, 0, 0, 1479 },
    { 2, "Drop size", D5NEXT_ISOLATED, Rain1_parameters_2, sizeof(Rain1_parameters_2) / sizeof(short), false, 0, 0, 1771 },
    { 3, "Drop smoothness", D5NEXT_ISOLATED, Rain1_parameters_3, sizeof(Rain1_parameters_3) / sizeof(short), false, 0, 0, 2051 },
};
static const D5NextFlagInfo Rain1_flags[] =
{
    { 0x0002, "Reverse direction", 2269, D5NEXT_ISOLATED },
    { 0x0008, "Random color", 2369, D5NEXT_ISOLATED },
    { 0x0010, "Snow", 2451, D5NEXT_ISOLATED },
};
static const short Rain2_parameters_0[] = { 50, 64 };
static const short Rain2_parameters_1[] = { 3, 2 };
static const short Rain2_parameters_2[] = { 10, 38 };
static const short Rain2_parameters_3[] = { 15, 50 };
static const D5NextParameterInfo Rain2_parameters[] =
{
    { 0, "Drop speed", D5NEXT_ISOLATED, Rain2_parameters_0, sizeof(Rain2_parameters_0) / sizeof(short), false, 0, 0, 3527 },
    { 1, "Drop items", D5NEXT_ISOLATED, Rain2_parameters_1, sizeof(Rain2_parameters_1) / sizeof(short), false, 0, 0, 3681 },
    { 2, "Drop size", D5NEXT_ISOLATED, Rain2_parameters_2, sizeof(Rain2_parameters_2) / sizeof(short), false, 0, 0, 3989 },
    { 3, "Drop smoothness", D5NEXT_ISOLATED, Rain2_parameters_3, sizeof(Rain2_parameters_3) / sizeof(short), false, 0, 0, 4299 },
};
static const D5NextFlagInfo Rain2_flags[] =
{
    { 0x0002, "Reverse direction", 4425, D5NEXT_ISOLATED },
    { 0x0008, "Random color", 4545, D5NEXT_ISOLATED },
    { 0x0010, "Snow", 4641, D5NEXT_ISOLATED },
};
static const short Rain3_parameters_0[] = { 25, 61 };
static const short Rain3_parameters_1[] = { 3, 2 };
static const short Rain3_parameters_2[] = { 60, 28 };
static const short Rain3_parameters_3[] = { 15, 64 };
static const D5NextParameterInfo Rain3_parameters[] =
{
    { 0, "Drop speed", D5NEXT_ISOLATED, Rain3_parameters_0, sizeof(Rain3_parameters_0) / sizeof(short), false, 0, 0, 6293 },
    { 1, "Drop items", D5NEXT_ISOLATED, Rain3_parameters_1, sizeof(Rain3_parameters_1) / sizeof(short), false, 0, 0, 6319 },
    { 2, "Drop size", D5NEXT_ISOLATED, Rain3_parameters_2, sizeof(Rain3_parameters_2) / sizeof(short), false, 0, 0, 6561 },
    { 3, "Drop smoothness", D5NEXT_ISOLATED, Rain3_parameters_3, sizeof(Rain3_parameters_3) / sizeof(short), false, 0, 0, 6829 },
};
static const D5NextFlagInfo Rain3_flags[] =
{
    { 0x0002, "Reverse direction", 7015, D5NEXT_ISOLATED },
    { 0x0008, "Random color", 7139, D5NEXT_ISOLATED },
    { 0x0010, "Snow", 7205, D5NEXT_ISOLATED },
};
static const short Sound2_parameters_4[] = { 25, 55, 28, 32, 19 };
static const short Sound2_parameters_7[] = { 5, 10 };
static const short Sound2_parameters_0[] = { 0, 2 };
static const short Sound2_filters_0[] = { 1, 38 };
static const short Sound2_filters_1[] = { 1, 58 };
static const short Sound2_parameters_3[] = { 1, 2 };
static const short Sound2_parameters_1[] = { 100, 20 };
static const short Sound2_parameters_5[] = { 50, 55, 20 };
static const D5NextParameterInfo Sound2_parameters[] =
{
    { 4, "Premiere limite", D5NEXT_ISOLATED, Sound2_parameters_4, sizeof(Sound2_parameters_4) / sizeof(short), false, 0, 0, 11875 },
    { 7, "Peak hold time", D5NEXT_ISOLATED, Sound2_parameters_7, sizeof(Sound2_parameters_7) / sizeof(short), false, 0, 0, 18529 },
    { 0, "Borne minimale de la source", D5NEXT_ISOLATED, Sound2_parameters_0, sizeof(Sound2_parameters_0) / sizeof(short), false, 0, 0, 23767 },
    { 3, "Nombre de limites", D5NEXT_COMPATIBLE, Sound2_parameters_3, sizeof(Sound2_parameters_3) / sizeof(short), false, 0, 0, 12207 },
    { 1, "Borne maximale de la source", D5NEXT_COMPATIBLE, Sound2_parameters_1, sizeof(Sound2_parameters_1) / sizeof(short), false, 0, 0, 24645 },
    { 5, "Seconde limite", D5NEXT_COMPATIBLE, Sound2_parameters_5, sizeof(Sound2_parameters_5) / sizeof(short), false, 0, 0, 12207 },
};
static const D5NextFilterInfo Sound2_filters[] =
{
    { 0, "Rising values", D5NEXT_ISOLATED, Sound2_filters_0, sizeof(Sound2_filters_0) / sizeof(short), 23159 },
    { 1, "Falling values", D5NEXT_ISOLATED, Sound2_filters_1, sizeof(Sound2_filters_1) / sizeof(short), 23309 },
};
static const D5NextColorInfo Sound2_colors[] =
{
    { 2, "Couleur de la premiere plage", D5NEXT_COLOR_H | D5NEXT_COLOR_S, D5NEXT_ISOLATED },
    { 1, "Couleur de pic", D5NEXT_COLOR_H | D5NEXT_COLOR_S, D5NEXT_ISOLATED },
};
static const D5NextFlagInfo Sound2_flags[] =
{
    { 0x0008, "Reverse direction", 16279, D5NEXT_ISOLATED },
    { 0x0002, "Show bar", 16439, D5NEXT_ISOLATED },
    { 0x0004, "Show ranges", 16613, D5NEXT_ISOLATED },
    { 0x0001, "Fade ranges", 16795, D5NEXT_ISOLATED },
    { 0x0020, "Show peak value", 16953, D5NEXT_ISOLATED },
};
static const short Sound3_parameters_8[] = { 10, 58 };
static const short Sound3_parameters_0[] = { 0, 1, 5, 4, 3, 2 };
static const short Sound3_parameters_4[] = { 5, 3 };
static const short Sound3_parameters_1[] = { 0, 2 };
static const short Sound3_parameters_5[] = { 5, 3 };
static const short Sound3_parameters_2[] = { 0, 3 };
static const short Sound3_parameters_6[] = { 5, 3 };
static const short Sound3_parameters_3[] = { 0, 4 };
static const short Sound3_parameters_7[] = { 5, 3 };
static const D5NextParameterInfo Sound3_parameters[] =
{
    { 8, "Rotate color", D5NEXT_ISOLATED, Sound3_parameters_8, sizeof(Sound3_parameters_8) / sizeof(short), false, 0, 0, 31807 },
    { 0, "Effect 1: sous-mode", D5NEXT_ISOLATED, Sound3_parameters_0, sizeof(Sound3_parameters_0) / sizeof(short), false, 0, 0, 35831 },
    { 4, "Effect 1: Speed", D5NEXT_ISOLATED, Sound3_parameters_4, sizeof(Sound3_parameters_4) / sizeof(short), false, 0, 0, 37387 },
    { 1, "Effect 2: sous-mode", D5NEXT_ISOLATED, Sound3_parameters_1, sizeof(Sound3_parameters_1) / sizeof(short), false, 0, 0, 36149 },
    { 5, "Effect 2: Speed", D5NEXT_ISOLATED, Sound3_parameters_5, sizeof(Sound3_parameters_5) / sizeof(short), false, 0, 0, 37579 },
    { 2, "Effect 3: sous-mode", D5NEXT_ISOLATED, Sound3_parameters_2, sizeof(Sound3_parameters_2) / sizeof(short), false, 0, 0, 36531 },
    { 6, "Effect 3: Speed", D5NEXT_ISOLATED, Sound3_parameters_6, sizeof(Sound3_parameters_6) / sizeof(short), false, 0, 0, 37793 },
    { 3, "Effect 4: sous-mode", D5NEXT_ISOLATED, Sound3_parameters_3, sizeof(Sound3_parameters_3) / sizeof(short), false, 0, 0, 36859 },
    { 7, "Effect 4: Speed", D5NEXT_ISOLATED, Sound3_parameters_7, sizeof(Sound3_parameters_7) / sizeof(short), false, 0, 0, 37959 },
};
static const short Sound4_parameters_0[] = { 10, 43 };
static const short Sound4_parameters_2[] = { 2, 6 };
static const short Sound4_parameters_3[] = { 10, 43 };
static const short Sound4_parameters_4[] = { 50, 25 };
static const D5NextParameterInfo Sound4_parameters[] =
{
    { 0, "Rotate color", D5NEXT_ISOLATED, Sound4_parameters_0, sizeof(Sound4_parameters_0) / sizeof(short), false, 0, 0, 41121 },
    { 2, "Effect 2: Effect length", D5NEXT_ISOLATED, Sound4_parameters_2, sizeof(Sound4_parameters_2) / sizeof(short), false, 0, 0, 45473 },
    { 3, "Idle speed", D5NEXT_ISOLATED, Sound4_parameters_3, sizeof(Sound4_parameters_3) / sizeof(short), false, 0, 0, 45643 },
    { 4, "Activity speed", D5NEXT_ISOLATED, Sound4_parameters_4, sizeof(Sound4_parameters_4) / sizeof(short), false, 0, 0, 45869 },
};
static const D5NextFlagInfo Sound4_flags[] =
{
    { 0x0002, "Effect 1: Random color", 41471, D5NEXT_ISOLATED },
    { 0x0004, "Effect 2: Random color", 44657, D5NEXT_ISOLATED },
    { 0x0001, "Reverse direction", 45927, D5NEXT_ISOLATED },
};

static const D5NextEffectInfo effects[] =
{
    { 0x00, "Unused", false, false, nullptr, 0, nullptr, 0, nullptr, 0 },
    { 0x01, "FixedColor", true, false, nullptr, 0, nullptr, 0, FixedColor_colors, 1 },
    { 0x02, "Breathing", true, false, Breathing_parameters, 4, nullptr, 0, nullptr, 0 },
    { 0x03, "Rainbow", true, false, Rainbow_parameters, 2, Rainbow_flags, 1, nullptr, 0 },
    { 0x04, "Blink1", true, false, Blink1_parameters, 1, Blink1_flags, 4, Blink1_colors, 1 },
    { 0x05, "Blink2", true, false, Blink2_parameters, 1, Blink2_flags, 3, nullptr, 0 },
    { 0x07, "Sequential", true, false, Sequential_parameters, 4, Sequential_flags, 3, nullptr, 0 },
    { 0x08, "Visor1", true, false, Visor1_parameters, 3, Visor1_flags, 5, nullptr, 0 },
    { 0x09, "Visor2", true, false, Visor2_parameters, 3, Visor2_flags, 5, nullptr, 0 },
    { 0x0A, "Marquee1", true, false, Marquee1_parameters, 3, Marquee1_flags, 3, nullptr, 0 },
    { 0x0B, "Marquee2", true, false, Marquee2_parameters, 3, Marquee2_flags, 2, nullptr, 0 },
    { 0x0C, "Ambient", true, false, Ambient_parameters, 3, nullptr, 0, nullptr, 0 },
    { 0x0D, "Bar1", true, false, nullptr, 0, nullptr, 0, nullptr, 0, D5NEXT_HOST_UNKNOWN, false },
    { 0x0E, "Candle", true, false, Candle_parameters, 1, nullptr, 0, nullptr, 0 },
    { 0x0F, "Rain1", true, false, Rain1_parameters, 4, Rain1_flags, 3, nullptr, 0, D5NEXT_HOST_UNKNOWN, false },
    { 0x10, "Rain2", true, false, Rain2_parameters, 4, Rain2_flags, 3, nullptr, 0, D5NEXT_HOST_UNKNOWN, false },
    { 0x11, "Rain3", true, false, Rain3_parameters, 4, Rain3_flags, 3, nullptr, 0, D5NEXT_HOST_UNKNOWN, false },
    { 0x12, "Switch", true, false, nullptr, 0, nullptr, 0, nullptr, 0, D5NEXT_HOST_UNKNOWN, false },
    { 0x13, "RainbowShift", true, false, RainbowShift_parameters, 5, nullptr, 0, nullptr, 0 },
    { 0x14, "Sound1", true, true, nullptr, 0, nullptr, 0, nullptr, 0, D5NEXT_HOST_PROBABLE, false },
    { 0x15, "Sound2", true, true, Sound2_parameters, 6, Sound2_flags, 5, Sound2_colors, 2, D5NEXT_HOST_PROBABLE, false, Sound2_filters, 2 },
    { 0x16, "Sound3", true, true, Sound3_parameters, 9, nullptr, 0, nullptr, 0, D5NEXT_HOST_PROBABLE, false },
    { 0x17, "Sound4", true, true, Sound4_parameters, 4, Sound4_flags, 3, nullptr, 0, D5NEXT_HOST_PROBABLE, false },
    { 0x18, "AmbientPx", true, true, nullptr, 0, nullptr, 0, nullptr, 0, D5NEXT_HOST_OBSERVED, false },
};

unsigned int D5NextEffects::Count()
{
    return sizeof(effects) / sizeof(effects[0]);
}

const D5NextEffectInfo* D5NextEffects::At(unsigned int index)
{
    return index < Count() ? &effects[index] : nullptr;
}

const D5NextEffectInfo* D5NextEffects::Find(unsigned int id)
{
    for(unsigned int index = 0; index < Count(); index++)
    {
        if(effects[index].id == id)
        {
            return &effects[index];
        }
    }
    return nullptr;
}

const D5NextParameterInfo* D5NextEffects::Parameter(unsigned int mode, unsigned int index)
{
    const D5NextEffectInfo* effect = Find(mode);
    if(effect != nullptr)
    {
        for(unsigned int item = 0; item < effect->parameter_count; item++)
        {
            if(effect->parameters[item].index == index)
            {
                return &effect->parameters[item];
            }
        }
    }
    return nullptr;
}

const D5NextFlagInfo* D5NextEffects::Flag(unsigned int mode, unsigned int mask)
{
    const D5NextEffectInfo* effect = Find(mode);
    if(effect != nullptr)
    {
        for(unsigned int item = 0; item < effect->flag_count; item++)
        {
            if(effect->flags[item].mask == mask)
            {
                return &effect->flags[item];
            }
        }
    }
    return nullptr;
}

const D5NextColorInfo* D5NextEffects::Color(unsigned int mode, unsigned int index)
{
    const D5NextEffectInfo* effect = Find(mode);
    if(effect != nullptr)
    {
        for(unsigned int item = 0; item < effect->color_count; item++)
        {
            if(effect->colors[item].index == index)
            {
                return &effect->colors[item];
            }
        }
    }
    return nullptr;
}
