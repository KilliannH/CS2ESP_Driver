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

// APC state structure for KeStackAttachProcess
typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PKPROCESS  Process;
    UCHAR      KernelApcInProgress;
    UCHAR      KernelApcPending;
    UCHAR      UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

NTKERNELAPI VOID KeStackAttachProcess(
    PRKPROCESS   PROCESS,
    PRKAPC_STATE ApcState
);

NTKERNELAPI VOID KeUnstackDetachProcess(
    PRKAPC_STATE ApcState
);