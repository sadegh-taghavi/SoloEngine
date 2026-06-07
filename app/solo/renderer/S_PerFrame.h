#pragma once
#include "solo/math/S_Math.h"

namespace solo
{

struct S_PerFrameData
{
    glm::mat4 VP;
    float     time      = 0.f;
    float     pad[3]    = {};
};
static_assert(sizeof(S_PerFrameData) == 80, "S_PerFrameData must be 80 bytes");

}
