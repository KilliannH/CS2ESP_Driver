#include "Memory.h"

NTSTATUS SafeReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size, PSIZE_T BytesRead) {
    if (BytesRead) {
        *BytesRead = 0;
    }

    if (!Process || !Address || !Buffer || Size == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    KAPC_STATE apcState;
    NTSTATUS   status = STATUS_SUCCESS;
    SIZE_T     copied = 0;

    // Attach to the target process so its user-space VAs are accessible
    KeStackAttachProcess(Process, &apcState);

    __try {
        // Validate the source range is readable (raises exception if not)
        ProbeForRead(Address, Size, 1);
        // Copy directly to our kernel buffer (always accessible)
        RtlCopyMemory(Buffer, Address, Size);
        copied = Size;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("[MemDemo] Exception reading 0x%p size=%llu: 0x%X\n",
                 Address, (ULONGLONG)Size, status);
    }

    KeUnstackDetachProcess(&apcState);

    if (BytesRead) {
        *BytesRead = copied;
    }

    return status;
}