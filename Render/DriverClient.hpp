#pragma once
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include "ESPData.hpp"  // Structures partagées

namespace ESP {
    class DriverClient {
    public:
        DriverClient();
        ~DriverClient();

        bool Initialize();
        void GetData(ESPData& outData);

    private:
        void UpdateLoop();

        HANDLE m_hDriver = INVALID_HANDLE_VALUE;
        std::atomic<bool> m_running{ false };
        std::thread m_workerThread;
        std::mutex m_dataMutex;
        ESPData m_currentData{};
    };
}