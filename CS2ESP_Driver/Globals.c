#include "Globals.h"

// Required when using float types in a WDK kernel-mode driver.
// Tells the CRT linker that floating-point is intentionally used.
int _fltused = 0;

PEPROCESS g_TargetProcess = NULL;
HANDLE g_TargetProcessId = NULL;
PVOID g_TargetAddress = NULL;
ULONGLONG g_TargetNonce = 0;
BOOLEAN g_TargetRegistered = FALSE;
FAST_MUTEX g_TargetLock;