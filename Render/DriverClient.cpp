#include "DriverClient.hpp"
#include <Windows.h>

namespace Demo {
    DriverClient::DriverClient() = default;

    DriverClient::~DriverClient() {
        ClearTarget();
        if (m_hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hDriver);
        }
    }

    bool DriverClient::Initialize() {
        m_hDriver = CreateFileW(
            L"\\\\.\\MemoryDemo",
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (m_hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        return true;
    }

    bool DriverClient::RegisterTarget(DWORD processId, const DemoMemoryBlock* block, std::uint64_t nonce) {
        if (m_hDriver == INVALID_HANDLE_VALUE || !block) {
            return false;
        }

        DemoRegisterRequest request{};
        request.ProcessId = processId;
        request.Address = reinterpret_cast<std::uint64_t>(block);
        request.Nonce = nonce;

        DWORD bytesReturned = 0;
        return DeviceIoControl(
            m_hDriver,
            IOCTL_DEMO_REGISTER_TARGET,
            &request,
            sizeof(request),
            nullptr,
            0,
            &bytesReturned,
            nullptr) != FALSE;
    }

    bool DriverClient::ReadBlock(DemoReadResult& outResult) {
        if (m_hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        ZeroMemory(&outResult, sizeof(outResult));

        return DeviceIoControl(
            m_hDriver,
            IOCTL_DEMO_READ_BLOCK,
            nullptr,
            0,
            &outResult,
            sizeof(outResult),
            &bytesReturned,
            nullptr) != FALSE &&
            bytesReturned == sizeof(outResult);
    }

    void DriverClient::ClearTarget() {
        if (m_hDriver == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD bytesReturned = 0;
        DeviceIoControl(
            m_hDriver,
            IOCTL_DEMO_CLEAR_TARGET,
            nullptr,
            0,
            nullptr,
            0,
            &bytesReturned,
            nullptr);
    }
}