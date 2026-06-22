#include "CS2Reader.h"
#include "Memory.h"

/*
 * Offsets statiques dans client.dll (dwXxx).
 * Source: cs2dumper / utilisateur.
 */
#define OFF_ENTITY_LIST             0x24E76A0ULL   // dwEntityList
#define OFF_LOCAL_PLAYER_CONTROLLER 0x2320720ULL   // dwLocalPlayerController
#define OFF_VIEW_MATRIX             0x2346B30ULL   // dwViewMatrix

/*
 * Offsets dans CCSPlayerController (CONFIRMES par le class dump).
 * CCSPlayerController::m_hPlayerPawn   = 2316 = 0x90C
 * CCSPlayerController::m_iPawnHealth   = 2328 = 0x918
 * CCSPlayerController::m_bPawnIsAlive  = 2324 = 0x914
 */
#define OFF_CTRL_HPAWN              0x90C
#define OFF_CTRL_PAWN_HEALTH        0x918
#define OFF_CTRL_PAWN_ALIVE         0x914

/*
 * Offset equipe : CCSPlayerController::m_iPendingTeamNum = 2112 = 0x840 (CONFIRME dump).
 * Valeur : 2 = Terroriste, 3 = CT. Lire comme BYTE.
 */
#define OFF_CTRL_TEAM               0x840   // m_iPendingTeamNum (byte)

/* m_pGameSceneNode (C_BaseEntity) — confirme = 0x330 */
#define OFF_GAME_SCENE_NODE         0x330
/* CGameSceneNode::m_vecAbsOrigin = 200 = 0xC8 (CONFIRME par class dump) */
#define OFF_SCENE_ABS_ORIGIN        0xC8
/* m_iHealth sur le pawn (C_BaseEntity) — source directe, pas de cache controller */
#define OFF_PAWN_HEALTH             0x34C

/* Entity list : header 0x10 bytes + chunks, stride 0x70 par slot */
#define ENTITY_LIST_HEADER          0x10
#define ENTITY_STRIDE               0x70

/* Validation adresse user-space Windows x64 */
#define IS_VALID_USER_PTR(addr) \
    ((addr) >= 0x10000ULL && (addr) <= 0x00007FFFFFFFFFFFULL)

/* ---- Helpers ---- */
static NTSTATUS ReadMem(PEPROCESS proc, ULONGLONG addr, PVOID buf, SIZE_T size)
{
    if (!IS_VALID_USER_PTR(addr)) return STATUS_INVALID_ADDRESS;
    SIZE_T br = 0;
    return SafeReadProcessMemory(proc, (PVOID)(ULONG_PTR)addr, buf, size, &br);
}

static NTSTATUS ReadPtr(PEPROCESS proc, ULONGLONG addr, PULONGLONG out)
{
    *out = 0;
    return ReadMem(proc, addr, out, sizeof(ULONGLONG));
}

static NTSTATUS ReadInt32(PEPROCESS proc, ULONGLONG addr, PLONG out)
{
    *out = 0;
    return ReadMem(proc, addr, out, sizeof(LONG));
}

static NTSTATUS ReadFloat(PEPROCESS proc, ULONGLONG addr, float* out)
{
    *out = 0.0f;
    return ReadMem(proc, addr, out, sizeof(float));
}

static ULONGLONG GetSceneNode(PEPROCESS proc, ULONGLONG entityPtr)
{
    ULONGLONG sn = 0;
    ReadPtr(proc, entityPtr + OFF_GAME_SCENE_NODE, &sn);
    return IS_VALID_USER_PTR(sn) ? sn : 0;
}

/*
 * Resoudre un handle CS2 (m_hPlayerPawn etc.) vers le pointeur d'entite reel.
 * handle & 0x7FFF = index dans la liste d'entites.
 */
static ULONGLONG ResolveHandle(PEPROCESS proc, ULONGLONG entityListBase, LONG handle)
{
    if (handle == 0) return 0;
    ULONG idx    = (ULONG)handle & 0x7FFF;
    ULONG chunkN = (idx >> 9) & 0x7F;
    ULONG slotN  = idx & 0x1FF;

    /* +0x10 : le tableau de chunk pointers commence apres un header de 16 bytes */
    ULONGLONG chunkPtr = 0;
    if (!NT_SUCCESS(ReadPtr(proc, entityListBase + ENTITY_LIST_HEADER + chunkN * 8, &chunkPtr)))
        return 0;
    if (!IS_VALID_USER_PTR(chunkPtr)) return 0;

    ULONGLONG entPtr = 0;
    if (!NT_SUCCESS(ReadPtr(proc, chunkPtr + slotN * ENTITY_STRIDE, &entPtr)))
        return 0;
    return IS_VALID_USER_PTR(entPtr) ? entPtr : 0;
}

/* ---- Main reader ---- */
NTSTATUS ReadCS2Data(HANDLE pid, ULONGLONG clientBase, PCS2_ESP_DATA out)
{
    if (!out) return STATUS_INVALID_PARAMETER;
    if (!IS_VALID_USER_PTR(clientBase)) return STATUS_INVALID_PARAMETER;

    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[CS2ESP] PsLookupProcessByProcessId failed: 0x%X\n", status);
        return status;
    }

    RtlZeroMemory(out, sizeof(CS2_ESP_DATA));

    /* ---- ViewMatrix ---- */
    ReadMem(process, clientBase + OFF_VIEW_MATRIX, out->viewMatrix, sizeof(out->viewMatrix));

    /* ---- EntityList base ---- */
    ULONGLONG entityListBase = 0;
    if (!NT_SUCCESS(ReadPtr(process, clientBase + OFF_ENTITY_LIST, &entityListBase))
        || !IS_VALID_USER_PTR(entityListBase)) {
        DbgPrint("[CS2ESP] entityListBase invalid\n");
        ObDereferenceObject(process);
        return STATUS_SUCCESS;
    }

    /* ---- Local player controller ---- */
    ULONGLONG localCtrlPtr = 0;
    ReadPtr(process, clientBase + OFF_LOCAL_PLAYER_CONTROLLER, &localCtrlPtr);

    UCHAR localTeam = 0;
    ULONGLONG localPawnPtr = 0;

    if (IS_VALID_USER_PTR(localCtrlPtr)) {
        /* Lire equipe du joueur local (1 byte : 2=T, 3=CT) */
        ReadMem(process, localCtrlPtr + OFF_CTRL_TEAM, &localTeam, sizeof(UCHAR));

        LONG hLocalPawn = 0;
        ReadInt32(process, localCtrlPtr + OFF_CTRL_HPAWN, &hLocalPawn);
        localPawnPtr = ResolveHandle(process, entityListBase, hLocalPawn);

        if (IS_VALID_USER_PTR(localPawnPtr)) {
            LONG localHealth = 0;
            ReadInt32(process, localCtrlPtr + OFF_CTRL_PAWN_HEALTH, &localHealth);

            ULONGLONG sceneNode = GetSceneNode(process, localPawnPtr);
            out->localPlayer.valid  = TRUE;
            out->localPlayer.health = localHealth;
            out->localPlayer.team   = (LONG)localTeam;

            if (IS_VALID_USER_PTR(sceneNode)) {
                ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 0, &out->localPlayer.x);
                ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 4, &out->localPlayer.y);
                ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 8, &out->localPlayer.z);
                /* Note: DbgPrint ne supporte pas %%f en kernel — affiche coords en int */
                DbgPrint("[CS2ESP] local team=%u hp=%ld pos=(%ld,%ld,%ld)\n",
                         (ULONG)localTeam, localHealth,
                         (LONG)out->localPlayer.x,
                         (LONG)out->localPlayer.y,
                         (LONG)out->localPlayer.z);
            } else {
                DbgPrint("[CS2ESP] local sceneNode invalid\n");
            }
        }
    }

    /* ---- Enemy entities ---- */
    ULONG count = 0;

    for (ULONG i = 1; i < 64 && count < CS2_MAX_ENTITIES; i++) {
        /* i = slot dans le premier chunk (players 1-63, chunk 0) */
        ULONGLONG chunkPtr = 0;
        if (!NT_SUCCESS(ReadPtr(process, entityListBase + ENTITY_LIST_HEADER, &chunkPtr))
            || !IS_VALID_USER_PTR(chunkPtr)) break;

        ULONGLONG ctrlPtr = 0;
        if (!NT_SUCCESS(ReadPtr(process, chunkPtr + i * ENTITY_STRIDE, &ctrlPtr))
            || !IS_VALID_USER_PTR(ctrlPtr)) continue;

        /* Exclure le joueur local */
        if (ctrlPtr == localCtrlPtr) continue;

        /* Filtrage equipe d'abord — pas besoin du pawn pour ca */
        UCHAR team = 0;
        ReadMem(process, ctrlPtr + OFF_CTRL_TEAM, &team, sizeof(UCHAR));
        if (team != 2 && team != 3) continue;            /* pas encore assigne */
        if (localTeam != 0 && team == localTeam) continue; /* meme equipe */

        /* Resoudre pawn handle -> pawn entity ptr */
        LONG hPawn = 0;
        ReadInt32(process, ctrlPtr + OFF_CTRL_HPAWN, &hPawn);
        ULONGLONG pawnEntPtr = ResolveHandle(process, entityListBase, hPawn);
        if (!IS_VALID_USER_PTR(pawnEntPtr)) continue;

        /*
         * Sante : on lit d'abord sur le pawn directement (m_iHealth @ 0x334).
         * Le champ cache du controller (m_iPawnHealth) peut rester a 0
         * pendant quelques frames apres le spawn de debut de round,
         * alors que la vraie sante sur le pawn est deja a 100.
         */
        LONG health = 0;
        ReadInt32(process, pawnEntPtr + OFF_PAWN_HEALTH, &health);
        if (health <= 0 || health > 100) {
            /* Fallback : cache controller */
            ReadInt32(process, ctrlPtr + OFF_CTRL_PAWN_HEALTH, &health);
            if (health <= 0 || health > 100) continue;
        }

        /*
         * m_bPawnIsAlive peut aussi etre faux en tout debut de round.
         * On s'y fie seulement si la sante est invalide (deja verifie
         * ci-dessus), donc on ne skip plus sur ce flag pour ne pas
         * manquer les premières frames du spawn.
         */

        /* Scene node -> position */
        ULONGLONG sceneNode = GetSceneNode(process, pawnEntPtr);
        if (!IS_VALID_USER_PTR(sceneNode)) continue;

        float ex = 0.0f, ey = 0.0f, ez = 0.0f;
        ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 0, &ex);
        ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 4, &ey);
        ReadFloat(process, sceneNode + OFF_SCENE_ABS_ORIGIN + 8, &ez);

        /* Log premier ennemi trouve (pas de %%f en kernel) */
        if (count == 0) {
            DbgPrint("[CS2ESP] first enemy: team=%u hp=%ld pos=(%ld,%ld,%ld)\n",
                     (ULONG)team, health, (LONG)ex, (LONG)ey, (LONG)ez);
        }

        out->entities[count].x      = ex;
        out->entities[count].y      = ey;
        out->entities[count].z      = ez;
        out->entities[count].health = health;
        out->entities[count].team   = (LONG)team;
        out->entities[count].valid  = TRUE;
        count++;
    }

    out->entityCount = count;
    DbgPrint("[CS2ESP] enemies found: %lu\n", count);

    ObDereferenceObject(process);
    return STATUS_SUCCESS;
}
