#pragma once
#include <ntddk.h>
#include "Driver.h"

// Reads all CS2 player/entity data from the process identified by pid.
// clientBase is the base address of client.dll in the target process.
// Returns STATUS_SUCCESS on success, fills *out.
NTSTATUS ReadCS2Data(HANDLE pid, ULONGLONG clientBase, PCS2_ESP_DATA out);
