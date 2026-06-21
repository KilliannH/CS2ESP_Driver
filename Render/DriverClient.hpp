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
        void ClearTarget();

    private:
        HANDLE m_hDriver = INVALID_HANDLE_VALUE;
    };
}