// Rendering.hpp
#pragma once
#include <imgui.h>
#include "ESPData.hpp"

namespace ESP {
    class Rendering {
    public:
        void DrawESP(const ESPData& data, ImDrawList* drawList, int screenWidth, int screenHeight);

    private:
        bool WorldToScreen(const float* viewMatrix, const Vector3& pos, int screenWidth, int screenHeight, float& outX, float& outY);
        ImU32 GetHealthColor(int health);
    };
}