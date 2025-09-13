#include "heightmap_terrain.hpp"

#include <vector>

#include <fastnoiselite.h>

const std::string& TERRAIN_SHADER_PATH = "/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaders/terrain.glsl";

std::vector<u32> get_neighboring_edges(u32 index) {
    return {
        {index + 1},
        {index - 1}
    };
}

HeightMapTerrain::TerrainMesh HeightMapTerrain::generate_plane(glm::vec2 size, glm::vec2 subdivide) {
    glm::vec2 triangle_size = size / glm::vec2(subdivide);

    int column_size = subdivide.x + 1;
    int row_size = subdivide.y + 1;
    int vertex_count = column_size * row_size;

    std::vector<TerrainVertex> vertices;
    std::vector<u32> indices;

    vertices.reserve(vertex_count);
    indices.reserve(vertex_count);

    constexpr glm::vec3 default_dir = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr u16 default_yaw = atan2(default_dir.z, default_dir.x);
    constexpr u16 default_pitch = asin(default_dir.y);

    for (int x = 0; x < column_size; x++) {
        for (int y = 0; y < row_size; y++) {
            vertices.emplace_back(TerrainVertex {
                {triangle_size.x * static_cast<float>(x), 0.0f, triangle_size.y * static_cast<float>(y)},
                default_yaw,
                default_pitch
            });
        }
    }
    
    for (int x = 0; x < subdivide.x; x++) {
        if (x % 2 == 0) {
            // Even column: top to bottom
            for (int y = 0; y <= subdivide.y; y++) {
                indices.push_back((x + 1) * row_size + y);
                indices.push_back((x    ) * row_size + y);
            }
        } else {
            // Odd column: bottom to top — swap order to keep diagonal consistent
            for (int y = subdivide.y; y >= 0; y--) {
                indices.push_back((x    ) * row_size + y);
                indices.push_back((x + 1) * row_size + y);
            }
        }
        
        printf("%u, %u\n", *(indices.end()-2), indices.back());
    }

    return {std::move(vertices), std::move(indices)};
}

HeightMapTerrain::HeightMapTerrain(glm::vec2 size, glm::vec2 subdivide) : shader(TERRAIN_SHADER_PATH) {
    generate_plane(size, subdivide);
}
