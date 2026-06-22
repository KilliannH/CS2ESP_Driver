#pragma once
#include <cstddef>

namespace OFFSETS {
    constexpr std::ptrdiff_t dwEntityList = 0x24E76A0;
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x2341698;
    constexpr std::ptrdiff_t dwLocalPlayerController = 0x2320720;
    constexpr std::ptrdiff_t dwViewMatrix = 0x2346B30;

    // C_BaseEntity (confirmed)
    constexpr std::ptrdiff_t m_iHealth = 0x34C;
    constexpr std::ptrdiff_t m_iTeamNum = 0x3EB;
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;

    // CCSPlayerController
    constexpr std::ptrdiff_t m_hPlayerPawn = 0x90C;
    constexpr std::ptrdiff_t m_iPawnHealth = 0x918;
    constexpr std::ptrdiff_t m_iPendingTeamNum = 0x840;

    // CGameSceneNode
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
}
