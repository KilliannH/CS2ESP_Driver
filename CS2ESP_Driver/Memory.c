#include "Memory.h"

NTSTATUS SafeReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size, PSIZE_T BytesRead) {
    SIZE_T localBytesRead = 0;

    if (BytesRead) {
        *BytesRead = 0;
    }

    if (!Process || !Address || !Buffer || Size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = MmCopyVirtualMemory(
        Process,
        Address,
        PsGetCurrentProcess(),
        Buffer,
        Size,
        KernelMode,
        &localBytesRead);

    if (BytesRead) {
        *BytesRead = localBytesRead;
    }

    if (!NT_SUCCESS(status)) {
        DbgPrint("[MemDemo] MmCopyVirtualMemory failed: 0x%X, bytes=%llu\n", status, (ULONGLONG)localBytesRead);
        return status;
    }

    if (localBytesRead != Size) {
        DbgPrint("[MemDemo] Partial copy: requested=%llu, read=%llu\n", (ULONGLONG)Size, (ULONGLONG)localBytesRead);
        return STATUS_PARTIAL_COPY;
    }

    return STATUS_SUCCESS;
}