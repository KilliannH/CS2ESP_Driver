#pragma once
#include <ntddk.h>

NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize
);

NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(
    HANDLE ProcessId,
    PEPROCESS* Process
);