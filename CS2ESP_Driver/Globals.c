#include "Globals.h"

PEPROCESS g_TargetProcess = NULL;
ULONG64 g_ClientBase = 0;
ULONG64 g_ViewMatrixAddr = 0;
BOOLEAN g_IsInitialized = FALSE;

const GameOffsets g_Offsets = {
    .dwEntityList = 0x24E76A0,              // EntityList pointer offset
    .dwLocalPlayerController = 0x2320720,   // LocalPlayerController offset
    .dwLocalPlayerPawn = 0x2341698,         // LocalPlayerPawn offset
    .dwViewMatrix = 0x2346B30,              // ViewMatrix offset (fallback if scan fails)
    .m_iPawnHealth = 0x918,                 // CCSPlayerController.m_iPawnHealth
    .m_iHealth = 0x34C,                     // C_BaseEntity.m_iHealth
    .m_iTeamNum = 0x3EB,                    // C_BaseEntity.m_iTeamNum
    .m_iPendingTeamNum = 0x840,             // CCSPlayerController.m_iPendingTeamNum
    .m_hPlayerPawn = 0x90C,                 // CCSPlayerController.m_hPlayerPawn
    .m_pGameSceneNode = 0x330,              // C_BaseEntity.m_pGameSceneNode
    .m_vecAbsOrigin = 0xC8                  // CGameSceneNode.m_vecAbsOrigin
};