#pragma once
#include <Windows.h>
#include "DemoData.hpp"

namespace Demo {
    class DriverClient {
    public:
        DriverClient();
        ~DriverClient();

        bool Initialize();
        bool RegisterTarget(DWORD processId, const DemoMemoryBlock* block, std::uint64_t nonce);
        bool ReadBlock(DemoReadResult& outResult);
        bool ReadProcessMemory(DWORD processId, std::uint64_t address, void* buffer, std::uint32_t size, DWORD& bytesRead);
        bool GetCS2Data(DWORD processId, std::uint64_t clientBase, Cs2EspData& out);
        void ClearTarget();

    private:
        HANDLE m_hDriver = INVALID_HANDLE_VALUE;
    };
}