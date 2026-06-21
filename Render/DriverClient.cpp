#include "DriverClient.hpp"
#include <Windows.h>

namespace ESP {
    DriverClient::DriverClient() = default;

    DriverClient::~DriverClient() {
        m_running = false;
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
        if (m_hDriver != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hDriver);
        }
    }

    bool DriverClient::Initialize() {
        m_hDriver = CreateFileW(
            L"\\\\.\\CS2ESP",
            GENERIC_READ,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (m_hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        m_running = true;
        m_workerThread = std::thread(&DriverClient::UpdateLoop, this);
        return true;
    }

    void DriverClient::GetData(ESPData& outData) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        outData = m_currentData;
    }

    void DriverClient::UpdateLoop() {
        while (m_running) {
            ESPData tempData{};
            DWORD bytesReturned = 0;

            if (DeviceIoControl(
                m_hDriver,
                IOCTL_GET_ESP_DATA,
                nullptr,
                0,
                &tempData,
                sizeof(tempData),
                &bytesReturned,
                nullptr
            )) {
                std::lock_guard<std::mutex> lock(m_dataMutex);
                m_currentData = tempData;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}