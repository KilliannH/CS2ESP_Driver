#pragma once
#include <ntddk.h>
#include "ntapi.h"  // Include les déclarations manquantes

// --- Constantes ---
#define DRIVER_TAG 'CS2D'
#define MAX_ENEMIES 64
#define IOCTL_GET_ESP_DATA CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// --- Structures personnalisées ---
typedef struct _Vector3 {
    float x, y, z;
} Vector3;

typedef struct _EnemyInfo {
    Vector3 pos;
    int health;
    int team;
} EnemyInfo;

typedef struct _ESP_DATA {
    float viewMatrix[16];
    int enemyCount;
    EnemyInfo enemies[MAX_ENEMIES];
} ESP_DATA, * PESP_DATA;

// --- Offsets CS2 ---
typedef struct _GameOffsets {
    ULONG dwEntityList;
    ULONG dwLocalPlayerController;
    ULONG dwLocalPlayerPawn;
    ULONG dwViewMatrix;
    ULONG m_iPawnHealth;
    ULONG m_iHealth;
    ULONG m_iTeamNum;
    ULONG m_iPendingTeamNum;
    ULONG m_hPlayerPawn;
    ULONG m_pGameSceneNode;
    ULONG m_vecAbsOrigin;
} GameOffsets;