// Rendering.cpp
#include "Rendering.hpp"
#include <Windows.h>
#include <cstdio>
#include <cmath>

namespace ESP {
    bool Rendering::WorldToScreen(const float* viewMatrix, const Vector3& pos, int screenWidth, int screenHeight, float& outX, float& outY) {
        float clipX = viewMatrix[0] * pos.x + viewMatrix[1] * pos.y + viewMatrix[2] * pos.z + viewMatrix[3];
        float clipY = viewMatrix[4] * pos.x + viewMatrix[5] * pos.y + viewMatrix[6] * pos.z + viewMatrix[7];
        float clipW = viewMatrix[12] * pos.x + viewMatrix[13] * pos.y + viewMatrix[14] * pos.z + viewMatrix[15];

        if (clipW < 0.01f) {
            return false;
        }

        float ndcX = clipX / clipW;
        float ndcY = clipY / clipW;

        outX = (screenWidth / 2.0f) * (1.0f + ndcX);
        outY = (screenHeight / 2.0f) * (1.0f - ndcY);

        return true;
    }

    ImU32 Rendering::GetHealthColor(int health) {
        if (health > 100) health = 100;
        if (health < 0) health = 0;

        int red = static_cast<int>(255 * (1.0f - (health / 100.0f)));
        int green = static_cast<int>(255 * (health / 100.0f));
        return IM_COL32(red, green, 0, 255);
    }

    void Rendering::DrawESP(const ESPData& data, ImDrawList* drawList, int screenWidth, int screenHeight) {
        for (int i = 0; i < data.enemyCount; ++i) {
            const EnemyData& enemy = data.enemies[i];

            float feetX, feetY;
            if (!this->WorldToScreen(data.viewMatrix, enemy.pos, screenWidth, screenHeight, feetX, feetY)) {
                continue;
            }

            Vector3 headPos = { enemy.pos.x, enemy.pos.y, enemy.pos.z + 72.0f };
            float headX, headY;
            if (!this->WorldToScreen(data.viewMatrix, headPos, screenWidth, screenHeight, headX, headY)) {
                continue;
            }

            float height = feetY - headY;
            float width = height * 0.5f;
            float x = headX - (width / 2.0f);
            float y = headY;

            ImU32 boxColor = this->GetHealthColor(enemy.health);
            drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), boxColor, 0.0f, 0, 1.5f);

            float barHeight = height * (enemy.health / 100.0f);
            drawList->AddRectFilled(
                ImVec2(x - 6, y),
                ImVec2(x - 3, y + height),
                IM_COL32(0, 0, 0, 180)
            );
            drawList->AddRectFilled(
                ImVec2(x - 6, y + height - barHeight),
                ImVec2(x - 3, y + height),
                boxColor
            );

            char hpText[8];
            sprintf_s(hpText, "%d", enemy.health);
            drawList->AddText(
                ImVec2(x - 35, y),
                IM_COL32(255, 255, 255, 255),
                hpText
            );
        }
    }
}