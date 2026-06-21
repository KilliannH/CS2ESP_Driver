#include "DriverClient.hpp"
#include <Windows.h>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {
    std::uint64_t CreateNonce(const DemoMemoryBlock& block) {
        LARGE_INTEGER ticks{};
        QueryPerformanceCounter(&ticks);

        return (static_cast<std::uint64_t>(GetCurrentProcessId()) << 32) ^
            static_cast<std::uint64_t>(ticks.QuadPart) ^
            reinterpret_cast<std::uint64_t>(&block);
    }
}


int main() {
    DemoMemoryBlock demoBlock{};
    const std::uint64_t nonce = CreateNonce(demoBlock);

    demoBlock.Magic = DEMO_MEMORY_MAGIC;
    demoBlock.Version = DEMO_MEMORY_VERSION;
    demoBlock.Size = sizeof(DemoMemoryBlock);
    demoBlock.Nonce = nonce;
    demoBlock.Counter = 0;
    strcpy_s(demoBlock.Message, "Initial owned-process demo block");

    Demo::DriverClient driver;
    if (!driver.Initialize()) {
        std::cerr << "Failed to open \\\\.\\MemoryDemo. Load the driver first. Win32 error: "
            << GetLastError() << '\n';
        return 1;
    }

    std::cout << "Educational kernel memory demo\n";
    std::cout << "PID: " << GetCurrentProcessId() << '\n';
    std::cout << "Block address: 0x" << std::hex
        << reinterpret_cast<std::uintptr_t>(&demoBlock) << std::dec << '\n';
    std::cout << "Nonce: 0x" << std::hex << nonce << std::dec << '\n';

    if (!driver.RegisterTarget(GetCurrentProcessId(), &demoBlock, nonce)) {
        std::cerr << "IOCTL_DEMO_REGISTER_TARGET failed. Win32 error: "
            << GetLastError() << '\n';
        return 1;
    }

    for (LONG i = 1; i <= 10; ++i) {
        demoBlock.Counter = i;
        sprintf_s(demoBlock.Message, "Owned process counter update %ld", i);

        DemoReadResult result{};
        if (!driver.ReadBlock(result)) {
            std::cerr << "IOCTL_DEMO_READ_BLOCK failed. Win32 error: "
                << GetLastError() << '\n';
            return 1;
        }

        std::cout << "Read " << result.BytesRead
            << " bytes, NTSTATUS=0x" << std::hex << result.Status << std::dec
            << ", counter=" << result.Block.Counter
            << ", message=\"" << result.Block.Message << "\"\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    driver.ClearTarget();
    std::cout << "Demo completed.\n";
    return 0;
}