#include "heightmap_terrain.hpp"

#include <vector>

#include <vertex_buffer_layout.hpp>
#include <renderer.hpp>

#include <fastnoiselite.h>

const std::string& TERRAIN_SHADER_PATH = "/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaders/optimized.glsl";

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
    std::vector<u8> indices;

    vertices.reserve(vertex_count);
    indices.reserve(vertex_count);

    constexpr float PI = 3.14159265359;

    constexpr glm::vec3 dir = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr float yaw = atan2(dir.z, dir.x);
    constexpr float pitch = asin(dir.y);

    constexpr u16 yaw_u16   = u16((yaw / (2.0f * PI)) * 65535.0f + 0.5f);
    constexpr u16 pitch_u16 = u16((pitch / PI) * 65535.0f + 0.5f);

    constexpr u32 packed_data = (u32(pitch_u16) << 16) | yaw_u16;

    for (int x = 0; x < column_size; x++) {
        for (int y = 0; y < row_size; y++) {
            // {triangle_size.x * static_cast<float>(x), 0.0f, triangle_size.y * static_cast<float>(y)}
            vertices.emplace_back(TerrainVertex {
                0.0f,
                packed_data
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

HeightMapTerrain::HeightMapTerrain(glm::vec2 size, glm::vec2 subdivide) : shader(TERRAIN_SHADER_PATH, "mvp") {
    mesh = generate_plane(size, subdivide);

    vertex_buffer = VertexBuffer(mesh.vertices.data(), sizeof(TerrainVertex) * mesh.vertices.size());

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 1);
    layout.push(GL_UNSIGNED_INT, 1);

    vertex_array.add_buffer(vertex_buffer, layout);
    index_buffer = IndexBuffer(mesh.indices.data(), mesh.indices.size());

    glm::vec2 quad_size = size / glm::vec2(subdivide);

    shader.bind();
    shader.set_uniform_v2("quad_size", quad_size.x, quad_size.y);
    shader.set_uniform_uv2("chunk_size", subdivide.x + 1, subdivide.y + 1);
    shader.set_uniform_v4("color", 0.4f, 0.4f, 0.9f, 1.0f);
}

void HeightMapTerrain::bind() const {
    vertex_array.bind();
    index_buffer.bind();
    shader.bind();
}

void HeightMapTerrain::draw(const glm::mat4 &mvp) {
    shader.set_mvp(mvp);
    gl_call(glDrawElements(GL_TRIANGLE_STRIP, mesh.indices.size(), GL_UNSIGNED_BYTE, (const void*)0));
}
