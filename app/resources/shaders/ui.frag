#version 450

layout(location = 0) in vec2  inUV;
layout(location = 1) in vec4  inColor;
layout(location = 2) in float inMode;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;   // R8 SDF
layout(set = 0, binding = 1) uniform sampler2D spriteAtlas; // RGBA8

void main()
{
    if (inMode > 0.5)
    {
        float d = texture(fontAtlas, inUV).r;
        float w = fwidth(d) * 0.75 + 0.001;
        float alpha = smoothstep(0.5 - w, 0.5 + w, d);
        outColor = vec4(inColor.rgb, inColor.a * alpha);
    }
    else
    {
        outColor = texture(spriteAtlas, inUV) * inColor;
    }
}
