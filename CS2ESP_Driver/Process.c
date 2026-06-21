#include "Process.h"
#include "Memory.h"
#include "Globals.h"

NTSTATUS FindProcessByName(PWCHAR ProcessName, PEPROCESS* OutProcess) {
    DbgPrint("[CS2] FindProcessByName called for: %ws\n", ProcessName);

    NTSTATUS status;
    ULONG bufferSize = 0x100000; // 1MB
    PSYSTEM_PROCESS_INFORMATION buffer = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, bufferSize, DRIVER_TAG);
    if (!buffer) {
        DbgPrint("[CS2] Failed to allocate buffer for process info\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG returnLength = 0;
    status = ZwQuerySystemInformation(5, buffer, bufferSize, &returnLength); // 5 = SystemProcessInformation

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        ExFreePoolWithTag(buffer, DRIVER_TAG);
        buffer = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, returnLength, DRIVER_TAG);
        if (!buffer) return STATUS_INSUFFICIENT_RESOURCES;
        status = ZwQuerySystemInformation(5, buffer, returnLength, &returnLength);
    }

    if (!NT_SUCCESS(status)) {
        DbgPrint("[CS2] ZwQuerySystemInformation failed: 0x%X\n", status);
        ExFreePoolWithTag(buffer, DRIVER_TAG);
        return status;
    }

    PSYSTEM_PROCESS_INFORMATION current = buffer;
    int processCount = 0;
    while (current->NextEntryOffset != 0) {
        processCount++;
        if (current->ImageName.Buffer) {
            if (wcsstr(current->ImageName.Buffer, L"cs2.exe") || wcsstr(current->ImageName.Buffer, L"cs2_bin64.exe")) {
                DbgPrint("[CS2] Found matching process: %ws (PID: %p)\n", current->ImageName.Buffer, current->UniqueProcessId);
                status = PsLookupProcessByProcessId(current->UniqueProcessId, OutProcess);
                if (NT_SUCCESS(status)) {
                    DbgPrint("[CS2] Successfully got PEPROCESS for %ws\n", current->ImageName.Buffer);
                    ExFreePoolWithTag(buffer, DRIVER_TAG);
                    return STATUS_SUCCESS;
                } else {
                    DbgPrint("[CS2] PsLookupProcessByProcessId failed: 0x%X\n", status);
                }
            }
        }
        if (current->NextEntryOffset == 0) break;
        current = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)current + current->NextEntryOffset);
    }

    DbgPrint("[CS2] Process %ws not found among %d processes\n", ProcessName, processCount);
    ExFreePoolWithTag(buffer, DRIVER_TAG);
    return STATUS_NOT_FOUND;
}

ULONG64 GetModuleBase(PEPROCESS Process, PWCHAR ModuleName) {
    DbgPrint("[CS2] GetModuleBase called for: %ws\n", ModuleName);

    KAPC_STATE apcState;
    KeStackAttachProcess(Process, &apcState);

    PPEB peb = PsGetProcessPeb(Process);
    if (!peb) {
        DbgPrint("[CS2] Failed to get PEB\n");
        KeUnstackDetachProcess(&apcState);
        return 0;
    }
    DbgPrint("[CS2] Got PEB: %p\n", peb);

    PPEB_LDR_DATA ldr = (PPEB_LDR_DATA)peb->Ldr;
    if (!ldr) {
        DbgPrint("[CS2] PEB->Ldr is NULL\n");
        KeUnstackDetachProcess(&apcState);
        return 0;
    }
    DbgPrint("[CS2] Got LDR: %p\n", ldr);

    LIST_ENTRY* listHead = &ldr->InMemoryOrderModuleList;
    LIST_ENTRY* curr = listHead->Flink;

    int moduleCount = 0;
    while (curr != listHead) {
        PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(curr, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        moduleCount++;
        if (entry->BaseDllName.Buffer) {
            DbgPrint("[CS2] Module %d: %ws (Base: 0x%p)\n", moduleCount, entry->BaseDllName.Buffer, entry->DllBase);
            if (_wcsnicmp(entry->BaseDllName.Buffer, ModuleName, wcslen(ModuleName)) == 0) {
                ULONG64 base = (ULONG64)entry->DllBase;
                DbgPrint("[CS2] Found %ws at 0x%llX\n", ModuleName, base);
                KeUnstackDetachProcess(&apcState);
                return base;
            }
        }
        curr = curr->Flink;
    }

    DbgPrint("[CS2] Module %ws not found among %d modules\n", ModuleName, moduleCount);
    KeUnstackDetachProcess(&apcState);
    return 0;
}

NTSTATUS InitializeDriver() {
    DbgPrint("[CS2] InitializeDriver called. g_TargetProcess=%p, g_ClientBase=0x%llX, g_IsInitialized=%d\n", 
             g_TargetProcess, g_ClientBase, g_IsInitialized);

    if (g_TargetProcess && g_ClientBase) {
        DbgPrint("[CS2] Already initialized\n");
        return STATUS_SUCCESS;
    }

    DbgPrint("[CS2] Starting process search (max 30 iterations)...\n");
    for (int i = 0; i < 30; i++) {
        DbgPrint("[CS2] Iteration %d: Looking for cs2.exe\n", i);
        if (NT_SUCCESS(FindProcessByName(L"cs2.exe", &g_TargetProcess))) {
            DbgPrint("[CS2] Found cs2.exe! g_TargetProcess=%p\n", g_TargetProcess);
            for (int j = 0; j < 10; j++) {
                DbgPrint("[CS2] Module search iteration %d: Looking for client.dll\n", j);
                g_ClientBase = GetModuleBase(g_TargetProcess, L"client.dll");
                if (g_ClientBase) {
                    DbgPrint("[CS2] Found client.dll at 0x%llX\n", g_ClientBase);
                    g_IsInitialized = TRUE;
                    DbgPrint("[CS2] Initialization successful!\n");
                    return STATUS_SUCCESS;
                }
                DbgPrint("[CS2] client.dll not found yet, waiting 500ms...\n");
                LARGE_INTEGER delay = { .QuadPart = -5000000 }; // 500ms
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
            }
            DbgPrint("[CS2] client.dll not found after 10 attempts, retrying process search\n");
            ObDereferenceObject(g_TargetProcess);
            g_TargetProcess = NULL;
        } else {
            DbgPrint("[CS2] cs2.exe not found in iteration %d\n", i);
        }
        LARGE_INTEGER delay = { .QuadPart = -10000000 }; // 1s
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    DbgPrint("[CS2] InitializeDriver FAILED - could not find cs2.exe or client.dll\n");
    return STATUS_NOT_FOUND;
}