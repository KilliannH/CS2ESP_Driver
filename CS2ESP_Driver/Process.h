#pragma once
#include "Driver.h"

// Fonctions
NTSTATUS FindProcessByName(PWCHAR ProcessName, PEPROCESS* OutProcess);
ULONG GetModuleBase(PEPROCESS Process, PWCHAR ModuleName);
NTSTATUS InitializeDriver();