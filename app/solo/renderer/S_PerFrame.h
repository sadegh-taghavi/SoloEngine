#pragma once
#include "solo/math/S_Math.h"

namespace solo
{

struct S_PerFrameData
{
    glm::mat4 VP;
    float     time      = 0.f;
    float     rtShadows = 1.f; // 0/1 toggle for ray-query shadows
    float     pad[2]    = {};
    glm::vec4 lightDir  = glm::vec4(0.35f, 0.9f, 0.25f, 0.0f); // world-space, toward the light
};
static_assert(sizeof(S_PerFrameData) == 96, "S_PerFrameData must be 96 bytes");

}
