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

    bool DriverClient::ReadProcessMemory(DWORD processId, std::uint64_t address, void* buffer, std::uint32_t size, DWORD& bytesRead) {
        if (m_hDriver == INVALID_HANDLE_VALUE || !buffer || size == 0) {
            return false;
        }

        DemoReadMemoryRequest request{};
        request.ProcessId = processId;
        request.Address = address;
        request.Size = size;

        bytesRead = 0;
        return DeviceIoControl(
            m_hDriver,
            IOCTL_DEMO_READ_PROCESS_MEMORY,
            &request,
            sizeof(request),
            buffer,
            size,
            &bytesRead,
            nullptr) != FALSE;
    }

    bool DriverClient::GetCS2Data(DWORD processId, std::uint64_t clientBase, Cs2EspData& out) {
        if (m_hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        Cs2DataRequest request{};
        request.processId  = static_cast<std::uint64_t>(processId);
        request.clientBase = clientBase;

        DWORD bytesReturned = 0;
        ZeroMemory(&out, sizeof(out));

        return DeviceIoControl(
            m_hDriver,
            IOCTL_DEMO_GET_CS2_DATA,
            &request,
            sizeof(request),
            &out,
            sizeof(out),
            &bytesReturned,
            nullptr) != FALSE && bytesReturned == sizeof(Cs2EspData);
    }
}