#pragma once
#include "Driver.h"

extern PEPROCESS g_TargetProcess;
extern HANDLE g_TargetProcessId;
extern PVOID g_TargetAddress;
extern ULONGLONG g_TargetNonce;
extern BOOLEAN g_TargetRegistered;
extern FAST_MUTEX g_TargetLock;