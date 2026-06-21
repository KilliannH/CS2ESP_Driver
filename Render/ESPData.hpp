#pragma once
#include <cstdint>

// --- Structures partagées entre le driver et l'overlay ---
struct Vector3 {
    float x, y, z;
};

struct EnemyData {
    Vector3 pos;
    int health;
    int team;
};

struct ESPData {
    float viewMatrix[16];
    int enemyCount;
    EnemyData enemies[64];
};

// --- IOCTL ---
#define IOCTL_GET_ESP_DATA CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)