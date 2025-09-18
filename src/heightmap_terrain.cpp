#include "heightmap_terrain.hpp"

#include <vector>

#include <vertex_buffer_layout.hpp>
#include <renderer.hpp>

#include <indirect_commands.hpp>

#include <fastnoiselite.h>

const std::string& TERRAIN_SHADER_PATH = "/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaders/optimized.glsl";

std::vector<u32> get_neighboring_edges(u32 index) {
    return {
        {index + 1},
        {index - 1}
    };
}

HeightMapTerrain::HeightMapTerrain(glm::vec2 size, glm::uvec2 subdivide) : size(size), subdivide(subdivide), shader(TERRAIN_SHADER_PATH, "mvp") {
    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 1);
    layout.push(GL_UNSIGNED_INT, 1);

    generate_index_buffer();

    glm::vec2 quad_size = size / glm::vec2(subdivide);

    u32 chunk_vertex_count = (subdivide.x + 1) * (subdivide.y + 1);

    chunk_manager = std::move(HeightMapChunkManager(128, chunk_vertex_count, index_buffer.get_count()));
    vertex_array.bind();
    chunk_manager.bind();
    vertex_array.add_buffer(chunk_manager.get_chunk_buffer(), layout);

    shader.bind();
    shader.set_uniform_v2("quad_size", quad_size.x, quad_size.y);
    shader.set_uniform_uv2("chunk_size", subdivide.x + 1, subdivide.y + 1);
    shader.set_uniform_v4("color", 0.4f, 0.4f, 0.9f, 1.0f);

    for (int x = 0; x < 11; x++) {
        for (int y = 0; y < 11; y++) {
            chunk_manager.add_chunk(glm::vec<2, u16>(x, y));
        }
    }
}

void HeightMapTerrain::generate_index_buffer() {
    int row_size = subdivide.y + 1;

    std::vector<u8> indices;

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
    }

    index_buffer = IndexBuffer(indices.data(), indices.size());
}

void HeightMapTerrain::bind() const {
    vertex_array.bind();
    index_buffer.bind();
    shader.bind();
    chunk_manager.bind();
}

void HeightMapTerrain::draw(const glm::mat4 &mvp) {
    bind();
    size_t draw_command_count = chunk_manager.generate_draw_commands();
    shader.set_mvp(mvp);
    if (draw_command_count > 0) {
        gl_call(glMultiDrawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_BYTE, nullptr, draw_command_count, 0));
    }
}

HeightMapChunkManager::HeightMapChunkManager(int chunk_count, u32 chunk_vertex_count, u32 chunk_index_count) 
: chunk_count(chunk_count), chunk_vertex_count(chunk_vertex_count), chunk_index_count(chunk_index_count) {
    chunk_size = chunk_vertex_count * sizeof(TerrainVertex);

    chunk_buffer = VertexBuffer(nullptr, chunk_count * chunk_size);
    chunk_offset_buffer = Buffer<GL_SHADER_STORAGE_BUFFER>(nullptr, chunk_count * sizeof(glm::vec<2, u16>));
    command_buffer = Buffer<GL_DRAW_INDIRECT_BUFFER>(nullptr, chunk_count * sizeof(DrawElementsIndirectCommand));

    chunks.resize(chunk_count / 8);
    chunk_offsets.resize(chunk_count);
}

HeightMapChunkManager::ChunkMesh HeightMapChunkManager::generate_chunk(glm::vec2 size, glm::uvec2 subdivide) {
    glm::vec2 triangle_size = size / glm::vec2(subdivide);

    int column_size = subdivide.x + 1;
    int row_size = subdivide.y + 1;
    int vertex_count = column_size * row_size;

    std::vector<TerrainVertex> vertices;
    std::vector<u8> indices;

    vertices.reserve(vertex_count);
    indices.reserve(vertex_count);

    constexpr float PI = 3.14159265359;

    const glm::vec3 dir = glm::vec3(0.0f, 1.0f, 0.0f);
    const float yaw = atan2(dir.z, dir.x);
    const float pitch = asin(dir.y);

    const u16 yaw_u16   = u16((yaw / (2.0f * PI)) * 65535.0f + 0.5f);
    const u16 pitch_u16 = u16((pitch / PI) * 65535.0f + 0.5f);

    const u32 packed_data = (u32(pitch_u16) << 16) | yaw_u16;

    for (int x = 0; x < column_size; x++) {
        for (int y = 0; y < row_size; y++) {
            vertices.emplace_back(TerrainVertex {
                0.0f,
                packed_data
            });
        }
    }

    return {std::move(vertices)};
}

size_t HeightMapChunkManager::reserve_chunk() {
    for (size_t i = 0; i < chunks.size(); i++) {
        std::bitset<8>& current_byte = chunks[i];
        if (current_byte.all()) {
            continue;
        }
        for (size_t j = 0; j < 8; j++) {
            if (current_byte[j] == 0) {
                current_byte[j] = 1;
                return (i * 8) + j;
            }
        }
    }

    return -1;
}

std::vector<u16> HeightMapChunkManager::get_used_chunk_indeces() {
    std::vector<u16> used_chunks;
    used_chunks.reserve(chunk_count);

    for (size_t i = 0; i < chunks.size(); i++) {
        for (size_t j = 0; j < 8; j++) {
            if (chunks[i][j] == 1) {
                used_chunks.emplace_back((i * 8) + j);
            }
        }
    }

    return used_chunks;
}

void HeightMapChunkManager::add_chunk(glm::vec<2, u16> chunk_pos) {
    size_t idx = reserve_chunk();

    ChunkMesh mesh = generate_chunk(glm::vec2(10.0f), glm::uvec2(10));

    chunk_offsets[idx] = chunk_pos;
    chunk_buffer.set_data(mesh.vertices.data(), chunk_size, idx * chunk_size);
}

bool should_draw_chunk(u16 chunk_idx) {
    return true;
}

size_t HeightMapChunkManager::generate_draw_commands() {
    std::vector<DrawElementsIndirectCommand> commands;
    commands.reserve(chunk_count);

    std::vector<u16> used_chunks = get_used_chunk_indeces();

    for (u16 chunk_idx : used_chunks) {
        if (should_draw_chunk(chunk_idx)) {
            commands.emplace_back(DrawElementsIndirectCommand {
                .count = chunk_index_count,
                .instance_count = 1,
                .first_index = 0,
                .base_vertex = chunk_idx * chunk_vertex_count,
                .base_instance = 0,
            });
        }
    }

    chunk_offset_buffer.set_data(chunk_offsets.data(), chunk_offsets.size() * sizeof(glm::vec<2, u16>));
    command_buffer.set_data(commands.data(), commands.size() * sizeof(DrawElementsIndirectCommand));
    return commands.size();
}

void HeightMapChunkManager::bind() const {
    chunk_buffer.bind();
    chunk_offset_buffer.bind();
    gl_call(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunk_offset_buffer.get_id()));
    command_buffer.bind();
}
