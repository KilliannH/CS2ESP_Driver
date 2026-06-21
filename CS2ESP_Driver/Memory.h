#pragma once
#include "Driver.h"

NTSTATUS SafeReadProcessMemory(PEPROCESS Process, PVOID Address, PVOID Buffer, SIZE_T Size, PSIZE_T BytesRead);