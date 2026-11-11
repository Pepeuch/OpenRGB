/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "D5NextSpeedTest.h"
#include <cstdio>
#include <cstring>
#include <mbedtls/sha256.h>

namespace D5NextSpeedTest
{
std::string Digest(const unsigned char* bytes, std::size_t size)
{
    unsigned char result[32];
    if(mbedtls_sha256_ret(bytes,size,result,0) != 0) return {};
    char hex[65];
    for(unsigned int i=0;i<32;i++) std::snprintf(hex+2*i,3,"%02x",result[i]);
    return hex;
}

/* Independent reflected bit oracle, separate from the production CRC++ code. */
static unsigned short CRC(const unsigned char* bytes, std::size_t size)
{
    unsigned int value=0xFFFF;
    for(std::size_t i=0;i<size;i++)
    {
        value^=bytes[i];
        for(unsigned int bit=0;bit<8;bit++) value=(value>>1)^((value&1)?0xA001:0);
    }
    return (unsigned short)(value^0xFFFF);
}

D5NextResult Plan(const unsigned char* original, std::size_t size, unsigned char (&candidate)[809], Action action)
{
    if(action!=Action::Speed && action!=Action::BlueHue && action!=Action::ExternalBlueHue
    && action!=Action::Group7YellowHue) return D5NEXT_INVALID_ARGUMENT;
    auto result=D5NextProtocol::ValidateReport(original,size);
    if(result!=D5NEXT_OK) return result;
    if(CRC(original+1,806)!=((original[807]<<8)|original[808])) return D5NEXT_INVALID_CRC;
    const D5NextIdentity identity={0x0C70,0xF00E,1,1027};
    D5NextRGBpxConfiguration configuration;
    result=D5NextRGBpx::Decode(original,size,identity,configuration);
    if(result!=D5NEXT_OK) return result;
    /* Specific observed topology for this campaign, not a general firmware rule. */
    const unsigned int starts[]={0,13,33,48,63,78,0,0};
    const unsigned int counts[]={13,20,15,15,15,12,6,6};
    for(unsigned int i=0;i<8;i++)
    {
        const auto& group=configuration.groups[i];
        if(group.strip!=(i<6?0:1) || group.led_start!=starts[i] || group.count!=counts[i])
            return D5NEXT_PRECONDITION_FAILED;
    }
    if(configuration.groups[6].mode!=2 || configuration.groups[7].mode!=2
    || configuration.groups[6].values[0]!=(action==Action::Speed?60:49)) return D5NEXT_PRECONDITION_FAILED;
    D5NextRGBpxPatch patch={};
    patch.kind=D5NEXT_PATCH_PARAMETER;patch.group=6;patch.expected_mode=2;patch.index=0;patch.value=49;
    unsigned char built[809];
    if(action==Action::Speed)
        result=D5NextRGBpx::BuildPatch(original,size,identity,patch,built,sizeof(built));
    else
    {
        /* Capture11 establishes isolated g0/g6 Breathing HSV0.H edits.
         * G7 is a separately authorized field experiment using the same layout,
         * with no rendering assumption. Exact independent byte audit below.
         * None of these experiments extends OpenRGB's normal patch policy. */
        const unsigned int target=action==Action::ExternalBlueHue?0:action==Action::Group7YellowHue?7:6;
        auto group=configuration.groups[target];
        if(group.mode!=2 || group.colors[0].hue!=(target==6?1343:1279)
        || group.colors[0].saturation!=255 || group.colors[0].value!=255)
            return D5NEXT_PRECONDITION_FAILED;
        if(target!=6 && (configuration.groups[6].colors[0].hue!=1024
        || configuration.groups[6].colors[0].saturation!=255 || configuration.groups[6].colors[0].value!=255))
            return D5NEXT_PRECONDITION_FAILED;
        if(target==7 && (configuration.groups[0].mode!=2 || configuration.groups[0].colors[0].hue!=1024
        || configuration.groups[0].colors[0].saturation!=255 || configuration.groups[0].colors[0].value!=255))
            return D5NEXT_PRECONDITION_FAILED;
        group.colors[0].hue=target==7?256:1024; // Yellow/blue in the documented 1536-step HSV scale.
        std::memcpy(built,original,809);
        result=D5NextRGBpx::EncodeController(group,built+240+70*target,70);
        if(result==D5NEXT_OK)
        {
            unsigned short crc=D5NextProtocol::CalculateCRC(built+1,806);
            built[807]=(unsigned char)(crc>>8);built[808]=(unsigned char)crc;
        }
    }
    if(result!=D5NEXT_OK) return result;
    unsigned char expected[809];std::memcpy(expected,original,809);
    unsigned int first=action==Action::Speed?682:action==Action::BlueHue?706:action==Action::ExternalBlueHue?286:776;
    expected[first]=action==Action::Speed?0:action==Action::Group7YellowHue?1:4;
    expected[first+1]=action==Action::Speed?0x31:0;
    unsigned short crc=CRC(expected+1,806);
    expected[807]=(unsigned char)(crc>>8);expected[808]=(unsigned char)crc;
    /* Exact expected buffer comparison, not AuditPatch calling BuildPatch again. */
    for(unsigned int i=0;i<809;i++)
    {
        if(built[i]!=expected[i]) return D5NEXT_UNEXPECTED_CHANGE;
        if(i!=first && i!=first+1 && i!=807 && i!=808 && built[i]!=original[i])
            return D5NEXT_UNEXPECTED_CHANGE;
    }
    if(D5NextProtocol::ValidateReport(built,809)!=D5NEXT_OK) return D5NEXT_INVALID_CRC;
    std::memcpy(candidate,built,809);
    return D5NEXT_OK;
}

void PrintPlan(const unsigned char (&original)[809], const unsigned char (&candidate)[809], Action action)
{
    if(action==Action::Group7YellowHue)
        std::puts("g7 overlapping Pump Breathing02 HSV0.H 1279->256 (#FFFF00); g0/g6 blue preserved; rendering priority unknown");
    else if(action==Action::ExternalBlueHue)
        std::puts("g0 external13 Breathing02 HSV0.H 1279->1024 (#0000FF); g6 blue and g7 preserved; allocations unchanged");
    else
        std::printf("g6 Breathing02 %s; g0 external13; g6/g7 overlap six positions, rendering priority unknown\n",
                    action==Action::Speed?"Speed60->49":"HSV0.H 1343->1024 (#0000FF), Speed49 and S/V255 preserved");
    std::printf("CRC initial=%02X%02X calculated=%04X target=%02X%02X\n",original[807],original[808],
                CRC(original+1,806),candidate[807],candidate[808]);
    unsigned int count=0;
    for(unsigned int i=0;i<809;i++) if(original[i]!=candidate[i])
    { std::printf("offset %u: %02X -> %02X\n",i,original[i],candidate[i]);count++; }
    std::printf("changed bytes=%u; all other bytes identical; no SettingsSave\n",count);
}

int Run(Transport& transport, Mode mode, const std::string& expected_digest, std::string& fresh_digest, Action action)
{
    if(mode!=Mode::DryRun && mode!=Mode::SendOnce) return 4;
    if(!transport.Safe()) return 4;
    unsigned char original[809]={3};
    if(transport.Get(original)!=809) { std::puts("STOP fresh GET failed; no retry");return 4; }
    unsigned char candidate[809];
    auto result=Plan(original,809,candidate,action);
    if(result!=D5NEXT_OK) {std::printf("STOP plan: %s\n",D5NextProtocol::ResultString(result));return 4;}
    auto digest=Digest(original,809);
    if(digest.empty()) return 4;
    PrintPlan(original,candidate,action);
    if(mode==Mode::DryRun) {fresh_digest=digest;std::puts("DRY-RUN complete; zero SET");return 0;}
    if(expected_digest.empty() || digest!=expected_digest)
    {std::puts("STOP configuration differs from approved dry-run; no SET");return 4;}
    if(!transport.Safe() || std::fflush(stdout)!=0) return 4;
    const int sent=transport.Set(candidate); // Sole call, never repeated.
    if(sent!=809) {std::puts("STOP SET completion uncertain; no more commands or automatic restore");return 5;}
    if(!transport.Safe()) {std::puts("STOP cooling/interruption alarm after SET; no control GET");return 5;}
    unsigned char actual[809]={3};
    if(transport.Get(actual)!=809) {std::puts("STOP control GET failed; no retry or restore");return 5;}
    if(D5NextProtocol::ValidateReport(actual,809)!=D5NEXT_OK || std::memcmp(actual,candidate,809)!=0)
    {
        std::puts("STOP accepted configuration differs; no more commands or restore");
        for(unsigned int i=0;i<809;i++) if(actual[i]!=candidate[i]) std::printf("unexpected offset %u\n",i);
        return 5;
    }
    if(!transport.Safe()) return 5;
    std::puts("ONE SET and exact 809-byte readback verified; operator must confirm LEDs and cooling");
    return 0;
}
}
