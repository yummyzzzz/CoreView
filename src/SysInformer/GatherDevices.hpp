#pragma once

#ifndef CONFIGFLAG_DISABLED
#define CONFIGFLAG_DISABLED 0x00000001
#endif

#include "Includes.h"

namespace Peripheral {
    struct ConnectedDevice {
        std::wstring name;
        std::string connectionType;
        std::wstring category;
    };

    inline bool IsSystemOrNoiseDevice(const std::wstring& name) {
        const std::vector<std::wstring> blocklist = {
            L"AVRCP", L"Transport", L"Enumerator", L"Auflistung", L"Dienst", L"Service",
            L"Profile", L"Profil", L"RFCOMM", L"TDI", L"GATT", L"Attribute", L"Attribut",
            L"DFU", L"Intel", L"Realtek", L"MediaTek", L"Qualcomm", L"Integrated",
            L"Built-in", L"Internal", L"Root Hub", L"Host Controller", L"Radio", L"Adapter",
            L"Eingabegerät", L"Input Device", L"HID-compliant", L"HID-konformes", L"Zugriffsprofil",
            L"Generisch", L"Generic", L"Auflister", L"LE-Enumerator", L"HID-Gerät", L"HID Device",
            L"Bluetooth Device", L"Bluetooth-Gerät"
        };

        for (const auto& blocked : blocklist) {
            if (name.find(blocked) != std::wstring::npos) return true;
        }
        return false;
    }

    inline std::vector<ConnectedDevice> GatherConnectedDevices() {
        std::vector<ConnectedDevice> devices;

        HDEVINFO hDevInfo = SetupDiGetClassDevsW(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        if (hDevInfo == INVALID_HANDLE_VALUE) return devices;

        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
            // Check if device is started/active using SetupAPI instead of cfgmgr32
            DWORD status = 0;
            if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_CONFIGFLAGS, NULL, (PBYTE)&status, sizeof(status), NULL)) {
                // If flag read fails, continue to next device
            }

            // Skip disabled devices
            if (status & CONFIGFLAG_DISABLED) continue;

            WCHAR classBuf[256] = { 0 };
            if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_CLASS, NULL, (PBYTE)classBuf, sizeof(classBuf), NULL)) {
                continue;
            }

            std::wstring devClass = classBuf;
            if (devClass != L"AudioEndpoint" && devClass != L"Keyboard" &&
                devClass != L"Mouse" && devClass != L"HIDClass" &&
                devClass != L"Bluetooth" && devClass != L"Image" && devClass != L"Camera") {
                continue;
            }

            WCHAR nameBuf[512] = { 0 };
            if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)nameBuf, sizeof(nameBuf), NULL)) {
                if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)nameBuf, sizeof(nameBuf), NULL)) {
                    continue;
                }
            }

            std::wstring name = nameBuf;
            if (name.empty() || IsSystemOrNoiseDevice(name)) continue;

            WCHAR enumBuf[256] = { 0 };
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_ENUMERATOR_NAME, NULL, (PBYTE)enumBuf, sizeof(enumBuf), NULL);
            std::wstring enumerator = enumBuf;

            WCHAR hwIdBuf[512] = { 0 };
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hwIdBuf, sizeof(hwIdBuf), NULL);
            std::wstring hwId = hwIdBuf;

            std::string connectionType = "Unknown";
            if (enumerator.find(L"BTH") != std::wstring::npos || hwId.find(L"BTH") != std::wstring::npos) {
                connectionType = "Bluetooth";
            }
            else if (enumerator.find(L"USB") != std::wstring::npos || hwId.find(L"USB") != std::wstring::npos) {
                connectionType = "Wired (USB / Cable)";
            }
            else {
                continue;
            }

            bool exists = false;
            for (const auto& d : devices) {
                if (d.name == name) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                ConnectedDevice dev;
                dev.name = name;
                dev.connectionType = connectionType;
                dev.category = devClass;
                devices.push_back(dev);
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
        return devices;
    }
}