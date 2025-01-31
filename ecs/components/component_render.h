#pragma once

namespace ecs{ 
namespace render {
namespace data {

struct material {
  v4 Ambient;
  v4 Diffuse;
  v4 Specular;
  r32 Shininess;
};

enum material_type{
  MATERIAL_EMERALD,
  MATERIAL_JADE,
  MATERIAL_OBSIDIAN,
  MATERIAL_PEARL,
  MATERIAL_RUBY,
  MATERIAL_TURQUOISE,
  MATERIAL_BRASS,
  MATERIAL_BRONZE,
  MATERIAL_TIN,
  MATERIAL_POLISHED_BRONZE,
  MATERIAL_CHROME,
  MATERIAL_COPPER,
  MATERIAL_POLISHED_COPPER,
  MATERIAL_GOLD,
  MATERIAL_POLISHED_GOLD,
  MATERIAL_SILVER,
  MATERIAL_POLISHED_SILVER ,
  MATERIAL_BLACK_PLASTIC,
  MATERIAL_CYAN_PLASTIC,
  MATERIAL_GREEN_PLASTIC,
  MATERIAL_RED_PLASTIC,
  MATERIAL_WHITE_PLASTIC,
  MATERIAL_YELLOW_PLASTIC,
  MATERIAL_BLACK_RUBBER,
  MATERIAL_CYAN_RUBBER,
  MATERIAL_GREEN_RUBBER,
  MATERIAL_RED_RUBBER,
  MATERIAL_WHITE_RUBBER,
  MATERIAL_YELLOW_RUBBER,
  MATERIAL_COUNT
};

} // namespace data

data::material GetMaterial(u32 MaterialIndex) {
    local_persist data::material Materials[data::MATERIAL_COUNT]{
    { {0.0215f,    0.1745f,    0.0215f,   0.55f}, {0.07568f,    0.61424f,    0.07568f,    0.55f}, {0.633f,       0.727811f,    0.633f,      0.55f}, 128 * 0.6f},         // emerald
    { {0.135f,     0.2225f,    0.1575f,   0.95f}, {0.54f,       0.89f,       0.63f,       0.95f}, {0.316228f,    0.316228f,    0.316228f,   0.95f}, 128 * 0.1f},         // jade
    { {0.05375f,   0.05f,      0.06625f,  0.82f}, {0.18275f,    0.17f,       0.22525f,    0.82f}, {0.332741f,    0.328634f,    0.346435f,   0.82f}, 128 * 0.3f},         // obsidian
    { {0.25f,      0.20725f,   0.20725f,  1.00f}, {1.0f,        0.829f,      0.829f,      1.00f}, {0.296648f,    0.296648f,    0.296648f,   1.00f}, 128 * 0.088f},       // pearl
    { {0.1745f,    0.01175f,   0.01175f,  0.55f}, {0.61424f,    0.04136f,    0.04136f,    0.55f}, {0.727811f,    0.626959f,    0.626959f,   0.55f}, 128 * 0.6f},         // ruby
    { {0.1f,       0.18725f,   0.1745f,   0.80f}, {0.396f,      0.74151f,    0.69102f,    0.80f}, {0.297254f,    0.30829f,     0.306678f,   0.80f}, 128 * 0.1f},         // turquoise
    { {0.329412f,  0.223529f,  0.027451f, 1.00f}, {0.780392f,   0.568627f,   0.113725f,   1.00f}, {0.992157f,    0.941176f,    0.807843f,   1.00f}, 128 * 0.21794872f},  // brass
    { {0.2125f,    0.1275f,    0.054f,    1.00f}, {0.714f,      0.4284f,     0.18144f,    1.00f}, {0.393548f,    0.271906f,    0.166721f,   1.00f}, 128 * 0.2f},         // bronze
    { {0.105882f, 0.058824f, 0.113725f,   1.00f}, {0.427451f,   0.470588f,   0.541176f,   1.00f}, {0.333333f,    0.333333f,    0.521569f,  1.0f },  9.84615f},           // tin
    { {0.25f,     0.148f,    0.06475f,    1.00f}, {0.4f,        0.2368f,     0.1036f,     1.00f}, {0.774597f,    0.458561f,    0.200621f,  1.0f },  76.8f},              // polished_bronze
    { {0.25f,      0.25f,      0.25f,     1.00f}, {0.4f,        0.4f,        0.4f,        1.00f}, {0.774597f,    0.774597f,    0.774597f,   1.00f}, 128 * 0.6f},         // chrome
    { {0.19125f,   0.0735f,    0.0225f,   1.00f}, {0.7038f,     0.27048f,    0.0828f,     1.00f}, {0.256777f,    0.137622f,    0.086014f,   1.00f}, 128 * 0.1f},         // copper
    { {0.2295f,   0.08825f,  0.0275f,     1.00f}, {0.5508f,     0.2118f,     0.066f,      1.00f}, {0.580594f,    0.223257f,    0.0695701f, 1.0f },  51.2f},              // polished_copper
    { {0.24725f,   0.1995f,    0.0745f,   1.00f}, {0.75164f,    0.60648f,    0.22648f,    1.00f}, {0.628281f,    0.555802f,    0.366065f,   1.00f}, 128 * 0.4f},         // gold
    { {0.24725f,  0.2245f,   0.0645f,     1.00f}, {0.34615f,    0.3143f,     0.0903f,     1.00f}, {0.797357f,    0.723991f,    0.208006f,  1.0f},   83.2f},              // polished_gold
    { {0.19225f,   0.19225f,   0.19225f,  1.00f}, {0.50754f,    0.50754f,    0.50754f,    1.00f}, {0.508273f,    0.508273f,    0.508273f,   1.00f}, 128 * 0.4f},         // silver
    { {0.23125f,  0.23125f,  0.23125f,    1.00f}, {0.2775f,     0.2775f,     0.2775f,     1.00f}, {0.773911f,    0.773911f,    0.773911f,  1.0f },  89.6f},              // polished_silver 
    { {0.0f,       0.0f,       0.0f,      1.00f}, {0.01f,       0.01f,       0.01f,       1.00f}, {0.50f,        0.50f,        0.50f,       1.00f}, 128 * 0.25f},        // black_plastic
    { {0.0f,       0.1f,       0.06f,     1.00f}, {0.0f,        0.50980392f, 0.50980392f, 1.00f}, {0.50196078f,  0.50196078f,  0.50196078f, 1.00f}, 128 * 0.25f},        // cyan_plastic
    { {0.0f,       0.0f,       0.0f,      1.00f}, {0.1f,        0.35f,       0.1f,        1.00f}, {0.45f,        0.55f,        0.45f,       1.00f}, 128 * 0.25f},        // green_plastic
    { {0.0f,       0.0f,       0.0f,      1.00f}, {0.5f,        0.0f,        0.0f,        1.00f}, {0.7f,         0.6f,         0.6f,        1.00f}, 128 * 0.25f},        // red_plastic
    { {0.0f,       0.0f,       0.0f,      1.00f}, {0.55f,       0.55f,       0.55f,       1.00f}, {0.70f,        0.70f,        0.70f,       1.00f}, 128 * 0.25f},        // white_plastic
    { {0.0f,       0.0f,       0.0f,      1.00f}, {0.5f,        0.5f,        0.0f,        1.00f}, {0.60f,        0.60f,        0.50f,       1.00f}, 128 * 0.25f},        // yellow_plastic
    { {0.02f,      0.02f,      0.02f,     1.00f}, {0.01f,       0.01f,       0.01f,       1.00f}, {0.4f,         0.4f,         0.4f,        1.00f}, 128 * 0.078125f},    // black_rubber
    { {0.0f,       0.05f,      0.05f,     1.00f}, {0.4f,        0.5f,        0.5f,        1.00f}, {0.04f,        0.7f,         0.7f,        1.00f}, 128 * 0.078125f},    // cyan_rubber
    { {0.0f,       0.05f,      0.0f,      1.00f}, {0.4f,        0.5f,        0.4f,        1.00f}, {0.04f,        0.7f,         0.04f,       1.00f}, 128 * 0.078125f},    // green_rubber
    { {0.05f,      0.0f,       0.0f,      1.00f}, {0.5f,        0.4f,        0.4f,        1.00f}, {0.7f,         0.04f,        0.04f,       1.00f}, 128 * 0.078125f},    // red_rubber
    { {0.05f,      0.05f,      0.05f,     1.00f}, {0.5f,        0.5f,        0.5f,        1.00f}, {0.7f,         0.7f,         0.7f,        1.00f}, 128 * 0.078125f},    // white_rubber
    { {0.05f,      0.05f,      0.0f,      1.00f}, {0.5f,        0.5f,        0.4f,        1.00f}, {0.7f,         0.7f,         0.04f,       1.00f}, 128 * 0.078125f},    // yellow_rubber
    };
    return Materials[MaterialIndex%data::MATERIAL_COUNT];
}

struct component {
  u32 MeshHandle;
  u32 TextureHandle;
  data::material Material;
};

} // namespace render
} // namespace ecs