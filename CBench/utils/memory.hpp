/*================================================================================
This software is open source software available under the BSD-3 license.

Copyright (c) 2017, Los Alamos National Security, LLC.
All rights reserved.

Authors:
 - Pascal Grosset
 - Jesus Pulido
================================================================================*/

#ifndef _MEM_H_
#define _MEM_H_

#if defined(__unix__) || defined(__unix) || defined(unix)
#include <sys/sysinfo.h>
#include <unistd.h>
#endif // Linux

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#endif

#if defined(WIN32)
#include <windows.h>
#include "psapi.h" //MSVC Performance query
#endif // Windows

#include <stdio.h>
#include <iostream>


class Memory
{
    unsigned long before_size, usage_size;    // program size
    unsigned long before_rss,  usage_rss;    // resident set size

    void GetMemorySize(unsigned long &size, unsigned long &rss);

public:
    Memory();
    Memory(bool start);
    ~Memory() {};

    void start();
    void stop();


    unsigned long getMemorySizeInB() { return usage_size; }
    double getMemorySizeInKB() { return usage_size / 1024.0; }
    double getMemorySizeInMB() { return usage_size / (1024.0 * 1024.0); }

    double getMemoryInUseInB();
    double getMemoryInUseInKB();
    double getMemoryInUseInMB();


    unsigned long getMemoryRSSInB() { return usage_rss; }
    double getMemoryRSSInKB() { return usage_rss / 1024.0; }
    double getMemoryRSSInMB() { return usage_rss / (1024.0 * 1024.0); }

    // static data and methods
    static std::map<std::string, size_t> sizeOf;

    static bool allocate(void*& data, std::string datatype, size_t num_elems=1, int offset=0);
    static bool release(void*& data, std::string datatype);
};

/* static*/ std::map<std::string, size_t> Memory::sizeOf = {
    {   "int", sizeof(int)},
    {   "float", sizeof(float)},
    {  "double", sizeof(double)},
    {  "int8_t", sizeof(int8_t)},
    { "int16_t", sizeof(int16_t)},
    { "int32_t", sizeof(int32_t)},
    { "int64_t", sizeof(int64_t)},
    { "uint8_t", sizeof(uint8_t)},
    {"uint16_t", sizeof(uint16_t)},
    {"uint32_t", sizeof(uint32_t)},
    {"uint64_t", sizeof(uint64_t)}
};

inline Memory::Memory()
{
    before_size = usage_size = 0;
    before_rss  = usage_rss  = 0;
}


inline Memory::Memory(bool start)
{
    before_size = usage_size = 0;
    before_rss  = usage_rss  = 0;

    GetMemorySize(before_size, before_rss);
}



inline void Memory::start()
{
    GetMemorySize(before_size, before_rss);
}


inline void Memory::stop()
{
    unsigned long after_size, after_rss;
    GetMemorySize(after_size, after_rss);

    usage_size = after_size - before_size;
    usage_rss = after_rss - before_rss;
}


inline bool Memory::allocate(void*& data, std::string datatype, size_t num_elems, int offset) 
{
    if (datatype == "int")
        data = new int[num_elems + offset];
    else if (datatype == "float")
        data = new float[num_elems + offset];
    else if (datatype == "double")
        data = new double[num_elems + offset];
    else if (datatype == "int8_t")
        data = new int8_t[num_elems + offset];
    else if (datatype == "int16_t")
        data = new int16_t[num_elems + offset];
    else if (datatype == "int32_t")
        data = new int32_t[num_elems + offset];
    else if (datatype == "int64_t")
        data = new int64_t[num_elems + offset];
    else if (datatype == "uint8_t")
        data = new uint8_t[num_elems + offset];
    else if (datatype == "uint16_t")
        data = new uint16_t[num_elems + offset];
    else if (datatype == "uint32_t")
        data = new uint32_t[num_elems + offset];
    else if (datatype == "uint64_t")
        data = new uint64_t[num_elems + offset];
    else
        return false;

    return true;
}


inline bool Memory::release(void*& data, std::string datatype) 
{

    if (data == nullptr) // already deallocated!
        return true;

    if (datatype == "int")
        delete[](int *) data;
    else if (datatype == "float")
        delete[](float *) data;
    else if (datatype == "double")
        delete[](double *) data;
    else if (datatype == "int8_t")
        delete[](int8_t *) data;
    else if (datatype == "int16_t")
        delete[](int16_t *) data;
    else if (datatype == "int32_t")
        delete[](int32_t *) data;
    else if (datatype == "int64_t")
        delete[](int64_t *) data;
    else if (datatype == "uint8_t")
        delete[](uint8_t *) data;
    else if (datatype == "uint16_t")
        delete[](uint16_t *) data;
    else if (datatype == "uint32_t")
        delete[](uint32_t *) data;
    else if (datatype == "uint64_t")
        delete[](uint64_t *) data;
    else
        return false;

    data = nullptr;
    return true;
}


inline double Memory::getMemoryInUseInB()
{
    unsigned long after_size, after_rss;
    GetMemorySize(after_size, after_rss);

    return (after_size - before_size);
}


inline double Memory::getMemoryInUseInKB()
{
    unsigned long after_size, after_rss;
    GetMemorySize(after_size, after_rss);

    return (after_size - before_size) / (1024.0);
}


inline double Memory::getMemoryInUseInMB()
{
    unsigned long after_size, after_rss;
    GetMemorySize(after_size, after_rss);

    return (after_size - before_size) / (1024.0 * 1024.0);
}


#if defined(__unix__) || defined(__unix) || defined(unix)
// From VisIt avt/Pipeline/Pipeline/avtMemory.cpp
inline void Memory::GetMemorySize(unsigned long &size, unsigned long &rss)
{
    size = 0;
    rss  = 0;

    FILE *file = fopen("/proc/self/statm", "r");
    if (file == NULL)
        return;

    int count = fscanf(file, "%lu%lu", &size, &rss);
    if (count != 2)
    {
        fclose(file);
        return;
    }
    size *= (unsigned long)getpagesize();
    rss  *= (unsigned long)getpagesize();
    fclose(file);
}
#elif defined(WIN32)
inline void Memory::GetMemorySize(unsigned long &size, unsigned long &rss)
{
    //Virtual Memory by current process
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;

    //Physical Memory Used by Current Process
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;

    size = virtualMemUsedByMe;
    rss = physMemUsedByMe;
}
#elif defined(__APPLE__) && defined(__MACH__)
inline void Memory::GetMemorySize(unsigned long &size, unsigned long &rss)
{
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
                                    (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
        rss = (size_t)0L;        /* Can't access? */

    rss = (size_t) info.resident_size;
    size = (size_t) info.virtual_size;
}
#endif

#endif    // _MEM_H_2
