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
        Sleep(30000);
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
        Sleep(30000);
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
        Sleep(30000);
    }
}

class CPU {
    public:
        CPU() {
            // COM bileşenlerini başlatıyoruz
            HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(hr)) {
                std::cerr << "COM başlatılamadı." << std::endl;
            }

            // WMI servisine bağlanıyoruz
            hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
            if (FAILED(hr)) {
                std::cerr << "WMI servisine bağlanılamadı." << std::endl;
            }

            hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"), nullptr, nullptr, 0, 0, 0, 0, &pSvc);
            if (FAILED(hr)) {
                std::cerr << "WMI sunucusuna bağlanılamadı." << std::endl;
            }

            hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            if (FAILED(hr)) {
                std::cerr << "WMI proxy blanket hatası." << std::endl;
            }
        }

        ~CPU() {
            // COM bileşenlerini serbest bırakıyoruz
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
        }

        float getTemperature() {
            float temp = -1.0f;

            // WMI ile işlemci sıcaklığını okuyoruz
            IEnumWbemClassObject* pEnumerator = nullptr;
            HRESULT hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
            if (FAILED(hr)) {
                std::cerr << "Sıcaklık okunamadı." << std::endl;
                return temp;
            }

            IWbemClassObject* pclsObj = nullptr;
            ULONG uReturn = 0;
            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (uReturn == 0) {
                    break;
                }

                VARIANT vtProp;
                hr = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, nullptr, nullptr);
                if (FAILED(hr)) {
                    pclsObj->Release();
                    continue;
                }

                // Sıcaklık değerini alıyoruz
                temp = static_cast<float>(vtProp.intVal) / 10.0f;

                VariantClear(&vtProp);
                pclsObj->Release();
            }
            pEnumerator->Release();
            
            return temp;
        }
    private:
        IWbemLocator* pLoc = nullptr;
        IWbemServices* pSvc = nullptr;
    };
extern "C"
{
    __declspec(dllexport) float ReturnCpuTemp() {
        CPU cpu_temp;
        return cpu_temp.getTemperature();
        Sleep(30000);
    }
}
int main() {
    CpuUsage cpu;RamUsage ram;DiskUsage disk(L"C:\\");CPU cpu_temp;
    while (true) {
        std::cout << "CPU Usage: " << cpu.GetCpuUsage() << "%" << std::endl;
        std::cout << "RAM Usage: " << ram.GetRamUsage() << "%" << std::endl;
        std::cout << "Free Disk Space: " << disk.GetFreeDiskSpace() << "%" << std::endl;
        std::cout << "İşlemci sıcaklığı: " << cpu_temp.getTemperature() << "°C" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(30));
        Sleep(30000); // 30 saniye bekle
    }
    return 0;

}


