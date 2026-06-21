#include "Game.h"
#include "Memory.h"
#include "Process.h"

ULONG GetEntityFromHandle(ULONG entityList, ULONG handle) {
    if (!entityList || handle == 0xFFFFFFFF || handle == 0)
        return 0;

    ULONG chunkIdx = (handle & 0x7FFF) >> 9;
    ULONG slotIdx = handle & 0x1FF;

    ULONG chunkPtr = 0;
    if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(entityList + 8ULL * chunkIdx + 0x10), &chunkPtr, sizeof(chunkPtr))) || !chunkPtr)
        return 0;

    ULONG entity = 0;
    if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(chunkPtr + 0x70ULL * slotIdx), &entity, sizeof(entity))) || !entity)
        return 0;

    return entity;
}

ULONG ScanViewMatrix() {
    if (!g_ClientBase) return 0;

    const char* pat1 = "\x48\x8B\x05\x00\x00\x00\x00\x48\x89\x45\x00\xF3\x0F\x10\x0D";
    const char* msk1 = "xxx????xxx?xxxx";
    ULONG foundAddr = KernelPatternScan(g_TargetProcess, g_ClientBase, pat1, msk1, 15);

    if (foundAddr) {
        LONG rel = 0;
        if (NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(foundAddr + 3), &rel, sizeof(rel))))
            return foundAddr + 7 + rel;
    }

    const char* pat2 = "\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\xD3\xE8";
    const char* msk2 = "xxx????xxxx";
    foundAddr = KernelPatternScan(g_TargetProcess, g_ClientBase, pat2, msk2, 11);

    if (foundAddr) {
        LONG rel = 0;
        if (NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(foundAddr + 3), &rel, sizeof(rel))))
            return foundAddr + 7 + rel;
    }

    return 0;
}

VOID CollectEnemyData(PESP_DATA Data) {
    if (!g_IsInitialized && !NT_SUCCESS(InitializeDriver()))
        return;

    RtlZeroMemory(Data, sizeof(ESP_DATA));

    if (!g_ViewMatrixAddr) {
        g_ViewMatrixAddr = ScanViewMatrix();
        if (!g_ViewMatrixAddr) return;
    }

    if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)g_ViewMatrixAddr, Data->viewMatrix, sizeof(Data->viewMatrix)))) {
        g_ViewMatrixAddr = 0;
        return;
    }

    ULONG entityList = 0;
    if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(g_ClientBase + g_Offsets.dwEntityList), &entityList, sizeof(entityList))) || !entityList)
        return;

    ULONG localController = 0, localPawn = 0;
    ReadPrimitive(g_TargetProcess, (PVOID)(g_ClientBase + g_Offsets.dwLocalPlayerController), &localController, sizeof(localController));
    ReadPrimitive(g_TargetProcess, (PVOID)(g_ClientBase + g_Offsets.dwLocalPlayerPawn), &localPawn, sizeof(localPawn));

    UCHAR localTeam = 0;
    ReadPrimitive(g_TargetProcess, (PVOID)(localPawn + g_Offsets.m_iTeamNum), &localTeam, sizeof(localTeam));

    int enemyIndex = 0;
    for (int i = 1; i < 64 && enemyIndex < MAX_ENEMIES; i++) {
        ULONG controller = GetEntityFromHandle(entityList, i);
        if (!controller || controller == localController) continue;

        ULONG pawnHandle = 0;
        if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(controller + g_Offsets.m_hPlayerPawn), &pawnHandle, sizeof(pawnHandle))) || pawnHandle == 0xFFFFFFFF)
            continue;

        ULONG pawn = GetEntityFromHandle(entityList, pawnHandle);
        if (!pawn) continue;

        int hp = 0;
        if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(controller + g_Offsets.m_iPawnHealth), &hp, sizeof(hp))) || hp <= 0 || hp > 100) {
            if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(pawn + g_Offsets.m_iHealth), &hp, sizeof(hp))) || hp <= 0 || hp > 100)
                continue;
        }

        UCHAR team = 0;
        if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(controller + g_Offsets.m_iPendingTeamNum), &team, sizeof(team))) || (team != 2 && team != 3)) {
            if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(pawn + g_Offsets.m_iTeamNum), &team, sizeof(team))) || (team != 2 && team != 3))
                continue;
        }
        if (team == localTeam) continue;

        Vector3 pos = { 0 };
        ULONG sceneNode = 0;
        if (NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(pawn + g_Offsets.m_pGameSceneNode), &sceneNode, sizeof(sceneNode))) && sceneNode) {
            if (!NT_SUCCESS(ReadPrimitive(g_TargetProcess, (PVOID)(sceneNode + g_Offsets.m_vecAbsOrigin), &pos, sizeof(pos))))
                continue;
        }
        else continue;

        Data->enemies[enemyIndex].pos = pos;
        Data->enemies[enemyIndex].health = hp;
        Data->enemies[enemyIndex].team = team;
        enemyIndex++;
    }

    Data->enemyCount = enemyIndex;
}