#pragma once
#include <Windows.h>
#include <cstdint>

#define DEMO_MEMORY_MAGIC 0x4F4D45444D454D4BULL
#define DEMO_MEMORY_VERSION 1
#define DEMO_MESSAGE_BYTES 64

struct DemoMemoryBlock {
    std::uint64_t Magic;
    std::uint32_t Version;
    std::uint32_t Size;
    std::uint64_t Nonce;
    LONG Counter;
    char Message[DEMO_MESSAGE_BYTES];
};

struct DemoRegisterRequest {
    std::uint64_t ProcessId;
    std::uint64_t Address;
    std::uint64_t Nonce;
};

struct DemoReadMemoryRequest {
    std::uint64_t ProcessId;
    std::uint64_t Address;
    std::uint32_t Size;
};

struct DemoReadResult {
    NTSTATUS Status;
    std::uint32_t BytesRead;
    DemoMemoryBlock Block;
};

#define IOCTL_DEMO_REGISTER_TARGET      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_DEMO_READ_BLOCK           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_DEMO_CLEAR_TARGET         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_DEMO_READ_PROCESS_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_DEMO_GET_CS2_DATA         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_DATA)

// ---- CS2 ESP structures (must match Driver.h) ----
#define CS2_MAX_ENTITIES 64

#pragma pack(push, 1)
struct PlayerInfo {
    float   x, y, z;
    int32_t health;
    int32_t team;
    uint8_t valid;      // BOOLEAN in kernel (1 byte)
    uint8_t _pad[3];   // alignment padding (matches kernel _pad[3])
};

struct Cs2EspData {
    float      viewMatrix[16];
    PlayerInfo localPlayer;
    PlayerInfo entities[CS2_MAX_ENTITIES];
    uint32_t   entityCount;
};

struct Cs2DataRequest {
    uint64_t processId;
    uint64_t clientBase;
};
#pragma pack(pop)

