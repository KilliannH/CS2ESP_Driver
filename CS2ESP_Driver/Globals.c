#include "Globals.h"

PEPROCESS g_TargetProcess = NULL;
HANDLE g_TargetProcessId = NULL;
PVOID g_TargetAddress = NULL;
ULONGLONG g_TargetNonce = 0;
BOOLEAN g_TargetRegistered = FALSE;
FAST_MUTEX g_TargetLock;