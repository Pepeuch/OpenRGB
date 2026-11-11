/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "D5NextSpeedTest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned int checks;
#define CHECK(x) do {checks++;if(!(x)){std::fprintf(stderr,"FAIL speed line %d: %s\n",__LINE__,#x);std::exit(1);}} while(0)
static unsigned short Reference(const unsigned char* bytes,unsigned int size)
{
    unsigned int remainder=0xFFFF;
    for(unsigned int i=0;i<size;i++) for(unsigned int bit=0;bit<8;bit++)
    {
        unsigned int carry=((remainder>>15)&1)^((bytes[i]>>bit)&1);
        remainder=(remainder<<1)&0xFFFF;
        if(carry) remainder^=0x8005;
    }
    unsigned int reflected=0;
    for(unsigned int i=0;i<16;i++) reflected=(reflected<<1)|((remainder>>i)&1);
    return (unsigned short)(reflected^0xFFFF);
}
static void Fix(unsigned char* bytes)
{
    auto crc=Reference(bytes+1,806);bytes[807]=(unsigned char)(crc>>8);bytes[808]=(unsigned char)crc;
}
static void Fixture(unsigned char* bytes,unsigned int seed=19)
{
    for(unsigned int i=0;i<809;i++) bytes[i]=(unsigned char)(seed+73*i);
    bytes[0]=3;bytes[1]=0;bytes[2]=3;
    const unsigned int starts[]={0,13,33,48,63,78,0,0},counts[]={13,20,15,15,15,12,6,6};
    for(unsigned int g=0;g<8;g++)
    {unsigned int offset=240+70*g;bytes[offset]=g<6?0:1;bytes[offset+1]=starts[g];bytes[offset+2]=counts[g];}
    bytes[663]=2;bytes[733]=2;bytes[682]=0;bytes[683]=60;Fix(bytes);
}
static void BlueFixture(unsigned char* bytes,unsigned int seed=19)
{
    Fixture(bytes,seed);bytes[683]=49;
    for(unsigned int i=0;i<6;i++)
    {bytes[706+4*i]=0;bytes[707+4*i]=(unsigned char)i;bytes[708+4*i]=255;bytes[709+4*i]=255;}
    bytes[706]=5;bytes[707]=0x3F;Fix(bytes);
}
static void ExternalBlueFixture(unsigned char* bytes,unsigned int seed=19)
{
    BlueFixture(bytes,seed);bytes[706]=4;bytes[707]=0;bytes[243]=2;
    for(unsigned int i=0;i<6;i++)
    {bytes[286+4*i]=0;bytes[287+4*i]=(unsigned char)i;bytes[288+4*i]=255;bytes[289+4*i]=255;}
    bytes[286]=4;bytes[287]=255;Fix(bytes);
}
static void Group7YellowFixture(unsigned char* bytes,unsigned int seed=19)
{
    ExternalBlueFixture(bytes,seed);bytes[287]=0;
    for(unsigned int i=0;i<6;i++)
    {bytes[776+4*i]=0;bytes[777+4*i]=(unsigned char)i;bytes[778+4*i]=255;bytes[779+4*i]=255;}
    bytes[776]=4;bytes[777]=255;Fix(bytes);
}
class Fake:public D5NextSpeedTest::Transport
{
public:
    unsigned char report[809];
    int gets=0,sets=0,safe_calls=0,first_length=809,second_length=809,set_length=809,unsafe_at=0;
    bool corrupt_readback=false;
    Fake(){Fixture(report);}
    int Get(unsigned char (&output)[809]) override
    {
        gets++;std::memcpy(output,report,809);
        if(gets==2 && corrupt_readback) {output[100]^=1;Fix(output);}
        return gets==1?first_length:second_length;
    }
    int Set(const unsigned char (&input)[809]) override
    {sets++;CHECK(sets==1);std::memcpy(report,input,809);return set_length;}
    bool Safe() override {safe_calls++;return safe_calls!=unsafe_at;}
};
int main()
{
    CHECK(D5NextSpeedTest::Digest((const unsigned char*)"abc",3)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    for(unsigned int seed=0;seed<16;seed++)
    {
        unsigned char original[809],candidate[809];Group7YellowFixture(original,seed);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::Group7YellowHue)==D5NEXT_OK);
        CHECK(candidate[776]==1 && candidate[777]==0);
        for(unsigned int i=0;i<807;i++) CHECK(i==776 || i==777 || candidate[i]==original[i]);
        CHECK(Reference(candidate+1,806)==((candidate[807]<<8)|candidate[808]));
    }
    for(unsigned int offset:{286U,287U,288U,289U,706U,707U,708U,709U,730U,731U,732U,733U,776U,777U,778U,779U})
    {
        unsigned char original[809],candidate[809];Group7YellowFixture(original);
        original[offset]^=1;Fix(original);std::memset(candidate,0xA5,809);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::Group7YellowHue)!=D5NEXT_OK);
        for(unsigned char byte:candidate) CHECK(byte==0xA5);
    }
    for(unsigned int seed=0;seed<16;seed++)
    {
        unsigned char original[809],candidate[809];ExternalBlueFixture(original,seed);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::ExternalBlueHue)==D5NEXT_OK);
        CHECK(candidate[286]==4 && candidate[287]==0 && candidate[706]==4 && candidate[707]==0);
        for(unsigned int i=0;i<807;i++) CHECK(i==287 || candidate[i]==original[i]);
        CHECK(Reference(candidate+1,806)==((candidate[807]<<8)|candidate[808]));
    }
    for(unsigned int offset:{240U,241U,242U,243U,286U,287U,288U,289U,706U,707U,708U,709U})
    {
        unsigned char original[809],candidate[809];ExternalBlueFixture(original);
        original[offset]^=1;Fix(original);std::memset(candidate,0xA5,809);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::ExternalBlueHue)!=D5NEXT_OK);
        for(unsigned char byte:candidate) CHECK(byte==0xA5);
    }
    for(unsigned int seed=0;seed<16;seed++)
    {
        unsigned char original[809],candidate[809];BlueFixture(original,seed);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::BlueHue)==D5NEXT_OK);
        CHECK(candidate[706]==4 && candidate[707]==0 && candidate[683]==49);
        for(unsigned int i=0;i<807;i++) CHECK(i==706 || i==707 || candidate[i]==original[i]);
        CHECK(Reference(candidate+1,806)==((candidate[807]<<8)|candidate[808]));
    }
    for(unsigned int scenario=0;scenario<7;scenario++)
    {
        unsigned char original[809],candidate[809];BlueFixture(original);
        const unsigned int offsets[]={683,706,707,708,709,663,733};
        original[offsets[scenario]]^=1;Fix(original);std::memset(candidate,0xA5,809);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate,D5NextSpeedTest::Action::BlueHue)!=D5NEXT_OK);
        for(unsigned char byte:candidate) CHECK(byte==0xA5);
    }
    for(unsigned int seed=0;seed<16;seed++)
    {
        unsigned char original[809],candidate[809];Fixture(original,seed);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate)==D5NEXT_OK);
        CHECK(candidate[682]==0 && candidate[683]==0x31);
        for(unsigned int i=0;i<807;i++) CHECK(i==683 || original[i]==candidate[i]);
        CHECK(Reference(candidate+1,806)==((candidate[807]<<8)|candidate[808]));
    }
    for(unsigned int scenario=0;scenario<15;scenario++)
    {
        unsigned char original[809],candidate[809];Fixture(original);
        if(scenario==0) original[0]=2;
        if(scenario==1) original[2]=4;
        if(scenario==2) original[663]=3;
        if(scenario==3) original[683]=49;
        if(scenario==4) original[733]=0;
        if(scenario>=5 && scenario<13) original[240+70*(scenario-5)+2]^=1;
        if(scenario==13) original[682]=0xFF;
        if(scenario!=14) Fix(original);else original[20]^=1;
        std::memset(candidate,0xA5,809);
        CHECK(D5NextSpeedTest::Plan(original,809,candidate)!=D5NEXT_OK);
        for(unsigned char byte:candidate) CHECK(byte==0xA5);
    }
    unsigned char original[809],candidate[809];Fixture(original);
    CHECK(D5NextSpeedTest::Plan(nullptr,809,candidate)==D5NEXT_INVALID_ARGUMENT);
    for(unsigned int size=0;size<809;size++) CHECK(D5NextSpeedTest::Plan(original,size,candidate)==D5NEXT_INVALID_SIZE);
    for(auto action:{D5NextSpeedTest::Action::Speed,D5NextSpeedTest::Action::BlueHue,D5NextSpeedTest::Action::ExternalBlueHue,D5NextSpeedTest::Action::Group7YellowHue})
    for(unsigned int scenario=0;scenario<14;scenario++)
    {
        Fake io;
        if(action==D5NextSpeedTest::Action::BlueHue) BlueFixture(io.report);
        if(action==D5NextSpeedTest::Action::ExternalBlueHue) ExternalBlueFixture(io.report);
        if(action==D5NextSpeedTest::Action::Group7YellowHue) Group7YellowFixture(io.report);
        std::string digest=D5NextSpeedTest::Digest(io.report,809),fresh;
        auto mode=scenario==1?D5NextSpeedTest::Mode::DryRun:D5NextSpeedTest::Mode::SendOnce;
        if(scenario==2) io.unsafe_at=1;
        if(scenario==3) io.first_length=-1;
        if(scenario==4) io.first_length=808;
        if(scenario==5) io.report[20]^=1;
        if(scenario==6) digest=std::string(64,'0');
        if(scenario==7) io.unsafe_at=2;
        if(scenario==8) io.set_length=-1;
        if(scenario==9) io.set_length=808;
        if(scenario==10) io.unsafe_at=3;
        if(scenario==11) io.second_length=-1;
        if(scenario==12) io.second_length=808;
        if(scenario==13) io.corrupt_readback=true;
        int result=D5NextSpeedTest::Run(io,mode,digest,fresh,action);
        CHECK(result==((scenario<2)?0:(scenario<8?4:5)));
        CHECK(io.sets==((scenario==0 || scenario>=8)?1:0));
        CHECK(io.gets==(scenario==2?0:scenario==0 || scenario>=11?2:1));
        if(scenario==1) CHECK(fresh==digest);
    }
    std::printf("PASS dedicated Speed tool: %u assertions, synthetic transport only, no HID backend\n",checks);
}
