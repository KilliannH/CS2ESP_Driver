#pragma once
#include <ntddk.h>
#include "ntapi.h"

#define DRIVER_TAG 'DMeM'
#define DEMO_DEVICE_NAME L"\\Device\\MemoryDemo"
#define DEMO_SYMBOLIC_LINK L"\\DosDevices\\MemoryDemo"

#define DEMO_MEMORY_MAGIC 0x4F4D45444D454D4BULL
#define DEMO_MEMORY_VERSION 1
#define DEMO_MESSAGE_BYTES 64

#define IOCTL_DEMO_REGISTER_TARGET      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_DEMO_READ_BLOCK           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_DEMO_CLEAR_TARGET         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_DEMO_READ_PROCESS_MEMORY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_DEMO_GET_CS2_DATA         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_DATA)

typedef struct _DEMO_MEMORY_BLOCK {
    ULONGLONG Magic;
    ULONG Version;
    ULONG Size;
    ULONGLONG Nonce;
    LONG Counter;
    CHAR Message[DEMO_MESSAGE_BYTES];
} DEMO_MEMORY_BLOCK, * PDEMO_MEMORY_BLOCK;

typedef struct _DEMO_REGISTER_REQUEST {
    ULONGLONG ProcessId;
    ULONGLONG Address;
    ULONGLONG Nonce;
} DEMO_REGISTER_REQUEST, * PDEMO_REGISTER_REQUEST;

typedef struct _DEMO_READ_MEMORY_REQUEST {
    ULONGLONG ProcessId;
    ULONGLONG Address;
    ULONG Size;
} DEMO_READ_MEMORY_REQUEST, * PDEMO_READ_MEMORY_REQUEST;

typedef struct _DEMO_READ_RESULT {
    NTSTATUS Status;
    ULONG BytesRead;
    DEMO_MEMORY_BLOCK Block;
} DEMO_READ_RESULT, * PDEMO_READ_RESULT;

/* ---- CS2 ESP structures ---- */

#define CS2_MAX_ENTITIES 64

typedef struct _PLAYER_INFO {
    float     x;        // m_vecAbsOrigin
    float     y;
    float     z;
    LONG      health;   // m_iHealth
    LONG      team;     // m_iTeamNum
    BOOLEAN   valid;    // slot actif
    UCHAR     _pad[3];  // alignment
} PLAYER_INFO, * PPLAYER_INFO;

typedef struct _CS2_ESP_DATA {
    float       viewMatrix[16]; // 4x4 row-major
    PLAYER_INFO localPlayer;
    PLAYER_INFO entities[CS2_MAX_ENTITIES];
    ULONG       entityCount;
} CS2_ESP_DATA, * PCS2_ESP_DATA;

typedef struct _CS2_DATA_REQUEST {
    ULONGLONG ProcessId;
    ULONGLONG ClientBase; // base address of client.dll in the target process
} CS2_DATA_REQUEST, * PCS2_DATA_REQUEST;

