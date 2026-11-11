/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "D5NextReadOnly.h"
#include "D5NextSpeedTest.h"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <poll.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace fs=std::filesystem;
static volatile std::sig_atomic_t interrupted=0;
static void Interrupt(int) { interrupted=1; }
static void Require(bool condition,const char* message)
{ if(!condition) throw std::runtime_error(message); }
struct FD
{
    int value;
    explicit FD(int number):value(number) {}
    FD(FD&& other) noexcept:value(other.value) {other.value=-1;}
    ~FD() {if(value>=0) close(value);}
    FD(const FD&)=delete; FD& operator=(const FD&)=delete;
};
static std::string Text(const fs::path& path)
{
    std::string value;
    Require(D5NextReadOnly::ReadText(path,value),"required sysfs metadata unavailable");
    return value;
}
static long Number(const std::string& text)
{
    std::size_t used=0;long value=std::stol(text,&used);
    Require(used==text.size(),"invalid numeric argument or sensor value");return value;
}
struct Clients
{
    unsigned int inaccessible=0;
    bool conflict=false;
};
static Clients CheckClients(const D5NextDeviceInfo& device)
{
    Clients result;
    result.conflict=!D5NextReadOnly::FindControllerProcesses().empty();
    auto usb="/dev/bus/usb/";
    std::string bus=Text(device.usb_path/"busnum"),address=Text(device.usb_path/"devnum");
    while(bus.size()<3) bus="0"+bus;
    while(address.size()<3) address="0"+address;
    std::string usb_node=std::string(usb)+bus+"/"+address;
    for(const auto& process:fs::directory_iterator("/proc"))
    {
        auto pid=process.path().filename().string();
        if(pid.find_first_not_of("0123456789")!=std::string::npos || pid==std::to_string(getpid())) continue;
        std::error_code error;
        fs::directory_iterator descriptors(process.path()/"fd",error);
        if(error) {if(error==std::errc::permission_denied) result.inaccessible++;continue;}
        for(const auto& fd:descriptors)
        {
            auto target=fs::read_symlink(fd.path(),error);
            if(!error && (target==device.dev_node || target==usb_node)) result.conflict=true;
        }
    }
    return result;
}

class Cooling
{
    fs::path pump,temp;
    long minimum,maximum;
    std::atomic<bool> stopped{false},alarm{false};
    std::thread worker;
public:
    Cooling(const D5NextDeviceInfo& device,long min_rpm,long max_temp):minimum(min_rpm),maximum(max_temp)
    {
        for(const auto& entry:fs::directory_iterator(device.hid_path/"hwmon"))
        {
            if(Text(entry.path()/"name")!="d5next") continue;
            for(const auto& sensor:fs::directory_iterator(entry.path()))
            {
                auto name=sensor.path().filename().string();
                if(name.size()<6 || name.substr(name.size()-6)!="_label") continue;
                auto label=Text(sensor.path());
                if(label=="Pump speed") pump=entry.path()/(name.substr(0,name.size()-6)+"_input");
                if(label=="Coolant temp") temp=entry.path()/(name.substr(0,name.size()-6)+"_input");
            }
        }
        Require(!pump.empty() && !temp.empty(),"required cached pump/coolant sensors unavailable");
        Require(Sample(true),"cooling snapshot outside limits; no device commands");
    }
    bool Sample(bool print)
    {
        try
        {
            long rpm=Number(Text(pump)),mc=Number(Text(temp));
            if(print) std::printf("cached cooling snapshot (private): pump_rpm=%ld coolant_mC=%ld\n",rpm,mc);
            return rpm>0 && rpm>=minimum && mc>0 && mc<=maximum;
        }
        catch(...) {return false;}
    }
    void Start()
    {
        worker=std::thread([this]()
        {
            while(!stopped)
            {
                if(interrupted || !Sample(false)) {alarm=true;return;}
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        });
    }
    bool Safe() const {return !alarm && !interrupted;}
    void Stop() {stopped=true;if(worker.joinable()) worker.join();}
    ~Cooling() {Stop();}
};

class LinuxTransport:public D5NextSpeedTest::Transport
{
    FD fd;
    D5NextDeviceInfo device;
    Cooling& cooling;
    bool writable;
    unsigned int sends=0;
public:
    LinuxTransport(FD descriptor,const D5NextDeviceInfo& info,Cooling& monitor,bool send)
        :fd(std::move(descriptor)),device(info),cooling(monitor),writable(send) {}
    int Get(unsigned char (&report)[809]) override
    {return ioctl(fd.value,HIDIOCGFEATURE(809),report);}
    int Set(const unsigned char (&report)[809]) override
    {
        if(!writable || sends++!=0 || !Safe()) return -1;
        return ioctl(fd.value,HIDIOCSFEATURE(809),report); // Only SET API in this tool.
    }
    bool Safe() override
    {return cooling.Safe() && cooling.Sample(false) && !CheckClients(device).conflict;}
};

static int Run(int argc,char** argv)
{
    bool send=false,operator_confirmed=false,explicit_dry=false,blue=false,external_blue=false,yellow7=false;
    std::string approval;
    long min_rpm=0,max_temp=100000;
    for(int i=1;i<argc;i++)
    {
        std::string option=argv[i];
        if(option=="--dry-run") explicit_dry=true;
        else if(option=="--blue") blue=true;
        else if(option=="--external-blue") external_blue=true;
        else if(option=="--group7-yellow") yellow7=true;
        else if(option=="--send-once") send=true;
        else if(option=="--operator-confirmed") operator_confirmed=true;
        else if(option=="--approve" && i+1<argc) approval=argv[++i];
        else if(option=="--min-pump-rpm" && i+1<argc) min_rpm=Number(argv[++i]);
        else if(option=="--max-coolant-mc" && i+1<argc) max_temp=Number(argv[++i]);
        else throw std::runtime_error("unknown/incomplete option; default is dry-run");
    }
    Require(geteuid()!=0,"do not run this tool as root");
    Require(!(send && explicit_dry),"conflicting modes");
    Require((unsigned int)blue+(unsigned int)external_blue+(unsigned int)yellow7<=1,"conflicting target groups");
    Require(send || (!operator_confirmed && approval.empty() && min_rpm==0 && max_temp==100000),"send options cannot accompany dry-run");
    if(send) Require(operator_confirmed && approval.size()==64 && min_rpm>0 && max_temp>0 && max_temp<100000,
                     "send requires approved dry-run, operator confirmation and explicit cooling limits");
    umask(077);
    const fs::path directory=yellow7?".build/d5next-group7-yellow":external_blue?".build/d5next-external-blue":blue?".build/d5next-blue-8d":".build/d5next-lot8c";
    const std::string receipt_version=yellow7?"D5NEXT_GROUP7_YELLOW_V1":external_blue?"D5NEXT_EXTERNAL_BLUE_V1":blue?"D5NEXT_BLUE_V1":"D5NEXT_SPEED_V1";
    if(mkdir(directory.c_str(),0700)!=0) Require(errno==EEXIST,"private state directory creation failed");
    struct stat state;
    Require(lstat(directory.c_str(),&state)==0 && S_ISDIR(state.st_mode) && state.st_uid==geteuid()
            && (state.st_mode&0077)==0,"state directory must be private and owned, not a symlink");
    FD lock(open((directory/"transaction.lock").c_str(),O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600));
    Require(lock.value>=0 && flock(lock.value,LOCK_EX|LOCK_NB)==0,"another campaign tool holds the transaction lock");
    Require(!fs::exists(directory/"attempt-used"),"campaign attempt already consumed; no retry or restore");
    std::string approved_serial,expected_digest,expected_token;
    if(send)
    {
        FD receipt(open((directory/"dry-run.receipt").c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));
        Require(receipt.value>=0,"mandatory dry-run receipt absent");
        char buffer[4096];ssize_t size=read(receipt.value,buffer,sizeof(buffer));
        Require(size>0 && size<(ssize_t)sizeof(buffer),"invalid receipt size");
        std::string contents(buffer,(std::size_t)size);
        auto line=[&contents]() {auto pos=contents.find('\n');Require(pos!=std::string::npos,"invalid receipt");auto value=contents.substr(0,pos);contents.erase(0,pos+1);return value;};
        Require(line()==receipt_version,"wrong receipt version");
        approved_serial=line();expected_digest=line();expected_token=line();
        Require(contents.empty() && !approved_serial.empty() && expected_digest.size()==64 && approval==expected_token,
                "approval does not match mandatory dry-run");
        FD used(open((directory/"attempt-used").c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC|O_NOFOLLOW,0600));
        Require(used.value>=0,"attempt already consumed"); // Consumed even if a later precondition fails.
        FD state_directory(open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
        Require(state_directory.value>=0 && fsync(used.value)==0 && fsync(state_directory.value)==0,
                "cannot durably consume this attempt");
    }
    else Require(!fs::exists(directory/"dry-run.receipt"),"dry-run receipt already exists; no automatic replacement");
    auto devices=D5NextReadOnly::Discover();
    Require(devices.size()==1,"strict USB0C70:F00E/HID1 identity ambiguous or absent");
    const auto& device=devices[0];
    std::string serial=Text(device.usb_path/"serial");
    Require(!serial.empty() && serial.size()<255 && serial.find('\n')==std::string::npos,"USB serial unavailable");
    if(send) Require(serial==approved_serial,"serial differs from approved dry-run");
    D5NextDescriptor descriptor;std::vector<unsigned char> descriptor_bytes;
    Require(D5NextReadOnly::ReadDescriptor(device.hid_path/"report_descriptor",descriptor_bytes)
            && D5NextReadOnly::ParseDescriptor(descriptor_bytes,descriptor)
            && descriptor.feature_bits[3]==6464 && descriptor.input_bits[1]==1288,"descriptor framing mismatch");
    auto clients=CheckClients(device);
    std::printf("known/accessible competing clients=%s; inaccessible fd directories=%u (not general exclusivity)\n",
                clients.conflict?"YES":"none",clients.inaccessible);
    Require(!clients.conflict,"competing controller or hidraw/vendor USB handle");
    Cooling cooling(device,min_rpm,max_temp);
    struct stat before;
    Require(stat(device.dev_node.c_str(),&before)==0 && S_ISCHR(before.st_mode)
            && faccessat(AT_FDCWD,device.dev_node.c_str(),send?(R_OK|W_OK):R_OK,AT_EACCESS)==0,"HID permissions unavailable");
    FD handle(open(device.dev_node.c_str(),(send?O_RDWR:O_RDONLY)|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW));
    Require(handle.value>=0 && flock(handle.value,LOCK_EX|LOCK_NB)==0,"HID open or cooperative lock failed");
    struct stat after;struct hidraw_devinfo identity={};
    Require(fstat(handle.value,&after)==0 && before.st_ino==after.st_ino && before.st_rdev==after.st_rdev
            && ioctl(handle.value,HIDIOCGRAWINFO,&identity)==0 && identity.bustype==BUS_USB
            && identity.vendor==0x0C70 && (unsigned short)identity.product==0xF00E,"opened USB identity mismatch");
    char unique[256]={};
    int unique_size=ioctl(handle.value,HIDIOCGRAWUNIQ(sizeof(unique)),unique);
    Require(unique_size>0 && unique_size<256 && std::string(unique)==serial,"opened HID serial differs from sysfs");
    auto reopened=D5NextReadOnly::Discover();
    Require(reopened.size()==1 && reopened[0].hid_path==device.hid_path
            && reopened[0].interface_path==device.interface_path && reopened[0].dev_node==device.dev_node,
            "HID interface changed after open");
    struct pollfd ready={handle.value,POLLIN,0};unsigned char status[163];
    Require(poll(&ready,1,2000)==1 && (ready.revents&POLLIN) && !(ready.revents&(POLLERR|POLLHUP|POLLNVAL)),"passive status unavailable; no retry");
    Require(read(handle.value,status,sizeof(status))==162 && status[0]==1 && ((status[13]<<8)|status[14])==1027,
            "Input01 firmware/framing mismatch");
    std::puts("strict USB0C70:F00E HID1 serial matched (withheld), firmware1027 confirmed; no mode transition");
    if(send) cooling.Start();
    LinuxTransport transport(std::move(handle),device,cooling,send);
    std::signal(SIGINT,Interrupt);std::signal(SIGTERM,Interrupt);
    if(send)
    {
        std::printf("operator interruption: PID=%ld, SIGINT/Ctrl-C stops future commands; cannot recall an in-flight ioctl\n",(long)getpid());
        std::puts("5 seconds of cached cooling monitoring before the fresh GET/SET transaction");
        std::fflush(stdout);
        for(unsigned int tick=0;tick<20;tick++)
        {
            Require(cooling.Safe(),"cooling/interruption alarm during pre-observation");
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
    std::string digest;
    int result=D5NextSpeedTest::Run(transport,send?D5NextSpeedTest::Mode::SendOnce:D5NextSpeedTest::Mode::DryRun,expected_digest,digest,
                                  yellow7?D5NextSpeedTest::Action::Group7YellowHue:
                                  external_blue?D5NextSpeedTest::Action::ExternalBlueHue:
                                  blue?D5NextSpeedTest::Action::BlueHue:D5NextSpeedTest::Action::Speed);
    if(send && result==0)
    {
        std::puts("10 seconds cached cooling surveillance; no further HID commands");
        std::fflush(stdout);
        for(unsigned int tick=0;tick<40;tick++)
        {
            Require(cooling.Safe(),"cooling/interruption alarm during post-observation; no more commands");
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
    cooling.Stop();
    bool final_cooling=cooling.Sample(true) && cooling.Safe();
    Require(final_cooling,"cooling alarm/interruption; stop, no automatic recovery");
    if(result!=0) return result;
    if(!send)
    {
        unsigned char random[32];Require(getrandom(random,sizeof(random),0)==(ssize_t)sizeof(random),"receipt random source failed");
        std::string token_material=serial+digest+std::string((char*)random,sizeof(random));
        std::string token=D5NextSpeedTest::Digest((const unsigned char*)token_material.data(),token_material.size());
        std::string receipt=receipt_version+"\n"+serial+"\n"+digest+"\n"+token+"\n";
        FD file(open((directory/"dry-run.receipt").c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC|O_NOFOLLOW,0600));
        Require(file.value>=0 && write(file.value,receipt.data(),receipt.size())==(ssize_t)receipt.size()
                && fsync(file.value)==0,"dry-run receipt storage failed");
        std::printf("approval token (for a separate operator-authorized invocation): %s\n",token.c_str());
        std::puts("STOP for operator review: other controllers stopped, normal circuit, live monitoring/limits, immediate intervention, Aquasuite recovery and g6/g7 masking acceptance required");
    }
    return 0;
}
int main(int argc,char** argv)
{
    try {return Run(argc,argv);}
    catch(const std::exception& error) {std::fprintf(stderr,"STOP: %s; no retry or automatic recovery\n",error.what());return 4;}
}
