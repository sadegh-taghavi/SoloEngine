#pragma once
#include <cstdint>

// ui.bin — baked UI assets: SDF font atlas + 9-slice sprite atlas.
// Produced by soloresourcegenerator from a .ttf, consumed by S_UI.

static constexpr uint32_t UI_BIN_MAGIC   = 0x4E494255; // 'UBIN'
static constexpr uint32_t UI_BIN_VERSION = 1;

#pragma pack(push, 1)

struct UIBinHeader
{
    uint32_t magic;
    uint32_t version;

    uint32_t fontAtlasWidth;     // R8 SDF atlas
    uint32_t fontAtlasHeight;
    float    fontPixelHeight;    // size glyphs were baked at
    float    ascent;             // scaled, pixels at fontPixelHeight
    float    descent;
    float    lineGap;
    uint32_t glyphCount;

    uint32_t spriteAtlasWidth;   // RGBA8 atlas
    uint32_t spriteAtlasHeight;
    uint32_t spriteCount;

    uint64_t glyphOffset;        // UIBinGlyph[glyphCount]
    uint64_t spriteOffset;       // UIBinSprite[spriteCount]
    uint64_t fontPixelsOffset;   // uint8[w*h]
    uint64_t spritePixelsOffset; // uint8[w*h*4]
};

struct UIBinGlyph
{
    uint32_t codepoint;
    float    u0, v0, u1, v1;     // atlas UVs
    float    xoff, yoff;         // bitmap placement relative to baseline cursor
    float    xadvance;
    float    width, height;      // bitmap size in pixels
};

struct UIBinSprite
{
    char     name[32];
    float    u0, v0, u1, v1;
    float    borderL, borderT, borderR, borderB; // 9-slice insets in source pixels
    float    pixelWidth, pixelHeight;            // source sprite size
};

#pragma pack(pop)
