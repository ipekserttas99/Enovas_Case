#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#include <chrono>
#include <thread>

class CpuUsage {
public:
    CpuUsage() {
        prevIdleTime_.QuadPart = 0;
        prevKernelTime_.QuadPart = 0;
        prevUserTime_.QuadPart = 0;
    }

    ~CpuUsage() {}

    // CPU kullanım yüzdesi
    float GetCpuUsage() {
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == 0) {
            return -1;
        }

        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        ULONGLONG idleTicks = idle.QuadPart - prevIdleTime_.QuadPart;
        ULONGLONG totalTicks = (kernel.QuadPart + user.QuadPart) - (prevKernelTime_.QuadPart + prevUserTime_.QuadPart);
        float usagePercent = 100.0f * (1.0f - ((float)idleTicks / (float)totalTicks));

        prevIdleTime_ = idle;
        prevKernelTime_ = kernel;
        prevUserTime_ = user;

        return usagePercent;
    }

private:
    ULARGE_INTEGER prevIdleTime_;
    ULARGE_INTEGER prevKernelTime_;
    ULARGE_INTEGER prevUserTime_;
};

extern "C"
{
    __declspec(dllexport) float ReturnCpuUsage() {
        CpuUsage cpu;
        return cpu.GetCpuUsage();
    }
}

class RamUsage {
public:
    RamUsage() {
        memoryStatus_.dwLength = sizeof(memoryStatus_);
    }

    ~RamUsage() {}

    // RAM kullanım yüzdesi
    float GetRamUsage() {
        if (GlobalMemoryStatusEx(&memoryStatus_) == 0) {
            return -1;
        }

        float ramUsagePercent = 100.0f * ((float)(memoryStatus_.ullTotalPhys - memoryStatus_.ullAvailPhys) / (float)memoryStatus_.ullTotalPhys);

        return ramUsagePercent;
    }

private:
    MEMORYSTATUSEX memoryStatus_;
};

extern "C"
{
    __declspec(dllexport) float ReturnRamUsage() {
        RamUsage ram;
        return ram.GetRamUsage();
    }
}

class DiskUsage {
public:
    DiskUsage(std::wstring drivePath) {
        drivePath_ = drivePath;
        ZeroMemory(&freeBytesAvailable_, sizeof(freeBytesAvailable_));
        ZeroMemory(&totalNumberOfBytes_, sizeof(totalNumberOfBytes_));
        ZeroMemory(&totalNumberOfFreeBytes_, sizeof(totalNumberOfFreeBytes_));
    }

    ~DiskUsage() {}

    // Boş disk alanı yüzdesi
    float GetFreeDiskSpace() {
        ULARGE_INTEGER freeBytesAvailable;
        ULARGE_INTEGER totalNumberOfBytes;
        ULARGE_INTEGER totalNumberOfFreeBytes;

        if (GetDiskFreeSpaceEx(drivePath_.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes) == 0) {
            return -1;
        }

        float freeDiskSpacePercent = 100.0f * ((float)freeBytesAvailable.QuadPart / (float)totalNumberOfBytes.QuadPart);

        return freeDiskSpacePercent;
    }

private:
    std::wstring drivePath_;
    ULARGE_INTEGER freeBytesAvailable_;
    ULARGE_INTEGER totalNumberOfBytes_;
    ULARGE_INTEGER totalNumberOfFreeBytes_;
};

extern "C"
{
    __declspec(dllexport) float ReturnDiskUsage() {
        DiskUsage disk(L"C:\\");
        return disk.GetFreeDiskSpace();
    }
}

class CPU {
    public:
        CPU() {}

        ~CPU() {}

        double  getTemperature() {
            double temperature = 0.0;

            HMODULE hMod = LoadLibraryA("ntdll.dll");
            if (hMod) {
                typedef DWORD(__stdcall* NtQuerySystemInformation_t)(ULONG, PVOID, ULONG, PULONG);
                NtQuerySystemInformation_t NtQuerySystemInformation = (NtQuerySystemInformation_t)GetProcAddress(hMod, "NtQuerySystemInformation");
                if (NtQuerySystemInformation) {
                    struct SYSTEM_PROCESSOR_INFORMATION {
                        USHORT Reserved;
                        USHORT Architecture;
                        ULONG ProcessorLevel;
                        ULONG ProcessorRevision;
                        ULONG NumberOfProcessors;
                        ULONG ProcessorClock;
                        ULONG_PTR AffinityMask;
                        ULONG Reserved1[2];
                        ULONG MaxPriority;
                        ULONG MinPriority;
                        ULONG MaxAppPriority;
                        ULONG MinAppPriority;
                        ULONG SettablePriClassMask;
                        ULONG Reserved2;
                        ULONG ExceptionList;
                        ULONG Reserved3[2];
                        ULONG ActiveProcessors;
                        UCHAR Reserved4[20];
                    } spi;

                    NtQuerySystemInformation(0x05, &spi, sizeof(spi), NULL);
                    temperature = ((static_cast<double>(spi.Reserved4[0]) / 2.0) - 32 )/ 1.8000;
                }
                FreeLibrary(hMod);
            }

            return temperature;
            
            
        }
    
        
};
extern "C"
{
    __declspec(dllexport) float ReturnCpuTemp() {
        CPU cpu_temp;
        return cpu_temp.getTemperature();
    }
}
int main() {
    CpuUsage cpu;RamUsage ram;DiskUsage disk(L"C:\\");CPU cpu_temp;
    while (true) {
        std::cout << "CPU Usage: " << cpu.GetCpuUsage() << "%" << std::endl;
        std::cout << "RAM Usage: " << ram.GetRamUsage() << "%" << std::endl;
        std::cout << "Free Disk Space: " << disk.GetFreeDiskSpace() << "%" << std::endl;
        std::cout << "İşlemci sıcaklığı: " << cpu_temp.getTemperature() << "°C" << std::endl;
    }
    return 0;

}


