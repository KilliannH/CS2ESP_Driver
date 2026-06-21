#include "Memory.h"
#include "Globals.h"

NTSTATUS SafeReadMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size) {
    if (!Process || !Address || !Buffer || Size == 0)
        return STATUS_INVALID_PARAMETER;

    SIZE_T bytesRead = 0;
    return MmCopyVirtualMemory(Process, Address, PsGetCurrentProcess(), Buffer, Size, KernelMode, &bytesRead);
}

NTSTATUS ReadPrimitive(PEPROCESS Process, ULONG64 Address, PVOID OutBuffer, SIZE_T Size) {
    return SafeReadMemory(Process, (PVOID)Address, OutBuffer, Size);
}

ULONG64 KernelPatternScan(PEPROCESS Process, ULONG64 ModuleBase, const char* pattern, const char* mask, SIZE_T maskLen) {
    if (!Process || !ModuleBase || !pattern || !mask || maskLen == 0)
        return 0;

#define SCAN_CHUNK_SIZE 0x1000
    PUCHAR buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, SCAN_CHUNK_SIZE + maskLen, DRIVER_TAG);
    if (!buffer) return 0;

    ULONG64 current = ModuleBase;
    ULONG64 endAddr = ModuleBase + 0x500000; // 5MB

    while (current < endAddr) {
        if (!NT_SUCCESS(ReadPrimitive(Process, (PVOID)current, buffer, SCAN_CHUNK_SIZE + maskLen))) {
            current += SCAN_CHUNK_SIZE;
            continue;
        }

        for (SIZE_T i = 0; i < SCAN_CHUNK_SIZE; i++) {
            BOOLEAN found = TRUE;
            for (SIZE_T j = 0; j < maskLen; j++) {
                if (mask[j] == 'x' && buffer[i + j] != (UCHAR)pattern[j]) {
                    found = FALSE;
                    break;
                }
            }
            if (found) {
                ExFreePoolWithTag(buffer, DRIVER_TAG);
                return current + i;
            }
        }
        current += (SCAN_CHUNK_SIZE - maskLen + 1);
    }

    ExFreePoolWithTag(buffer, DRIVER_TAG);
    return 0;
}