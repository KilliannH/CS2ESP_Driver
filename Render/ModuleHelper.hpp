#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

namespace Demo {
    // Returns the base address of the specified module (by name) inside the target process.
    // Returns 0 on failure.
    inline std::uint64_t GetModuleBase(DWORD pid, const wchar_t* moduleName)
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        std::uint64_t base = 0;

        if (Module32FirstW(hSnap, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName) == 0) {
                    base = reinterpret_cast<std::uint64_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(hSnap, &me));
        }
        CloseHandle(hSnap);
        return base;
    }
}
