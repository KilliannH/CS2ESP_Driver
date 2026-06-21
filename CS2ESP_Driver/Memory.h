#pragma once
#include "Driver.h"

// Déclarations
NTSTATUS SafeReadMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size);
NTSTATUS ReadPrimitive(PEPROCESS Process, ULONG64 Address, PVOID OutBuffer, SIZE_T Size);
ULONG64 KernelPatternScan(PEPROCESS Process, ULONG64 ModuleBase, const char* pattern, const char* mask, SIZE_T maskLen);