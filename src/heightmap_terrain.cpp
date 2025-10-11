#include "heightmap_terrain.hpp"

#include <vector>
#include <span>

#include <vertex_buffer_layout.hpp>
#include <renderer.hpp>
#include <camera3d.hpp>

#include <indirect_commands.hpp>

const std::string& TERRAIN_SHADER_PATH = "/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaders/optimized.glsl";

const int CHUNKS_PER_AXIS = 64;

HeightMapTerrain::HeightMapTerrain(glm::vec2 size, glm::uvec2 subdivide) : size(size), subdivide(subdivide), shader(TERRAIN_SHADER_PATH, "mvp") {
    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 1);
    layout.push(GL_UNSIGNED_INT, 1);

    generate_index_buffer();

    glm::vec2 quad_size = size / glm::vec2(subdivide);

    chunk_manager = std::move(HeightMapChunkManager(CHUNKS_PER_AXIS*CHUNKS_PER_AXIS, subdivide, index_buffer.get_count()));
    vertex_array.bind();
    chunk_manager.bind();
    vertex_array.add_buffer(chunk_manager.get_chunk_buffer(), layout);

    shader.bind();
    shader.set_uniform_v2("quad_size", quad_size.x, quad_size.y);
    shader.set_uniform_uv2("chunk_size", subdivide.x + 1, subdivide.y + 1);
    //shader.set_uniform_v4("color", 0.4f, 0.4f, 0.9f, 1.0f);

    shader.set_uniform_v3("material.color", 0.4f, 0.4, 0.9f);
    //cube_shader.set_uniform_v3("u_material.specular", 1.0f, 1.0f, 1.0f);
    shader.set_uniform_1f("material.shininess", 32.0f);

    shader.set_uniform_v3("sun.direction", -1.0f, -1.0f, 0.5f);
    shader.set_uniform_v3("sun.diffuse", 1.0f, 1.0f, 0.9f);

    shader.set_uniform_mat4f("model", glm::mat4(1.0f));

    for (int x = 0; x < CHUNKS_PER_AXIS; x++) {
        for (int y = 0; y < CHUNKS_PER_AXIS; y++) {
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

u16 HeightMapTerrain::get_chunk_id_by_pos(glm::vec<2, i16> chunk_pos) const {
    return chunk_manager.get_chunk_id_by_pos(chunk_pos);
}

u16 HeightMapTerrain::get_chunk_id_by_world_pos(glm::vec2 world_pos) const {
    return chunk_manager.get_chunk_id_by_world_pos(world_pos);
}

float HeightMapTerrain::get_vertex_height_in_chunk_by_pos(u16 chunk_index, int x, int y) {
    return chunk_manager.get_vertex_height_in_chunk_by_pos(chunk_index, x, y);
}

float HeightMapTerrain::get_vertex_height_by_world_pos(glm::vec2 world_pos) {
    return chunk_manager.get_vertex_height_by_world_pos(world_pos);
}

void HeightMapTerrain::draw(const glm::mat4 &mvp, const Camera3D::Frustum& view_frustum) {
    bind();
    size_t draw_command_count = chunk_manager.generate_draw_commands(view_frustum);
    shader.set_mvp(mvp);
    if (draw_command_count > 0) {
        gl_call(glMultiDrawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_BYTE, nullptr, draw_command_count, 0));
    }
}

HeightMapChunkManager::HeightMapChunkManager(int chunk_count, glm::uvec2 subdivide, u32 chunk_index_count)
: chunk_count(chunk_count), chunk_index_count(chunk_index_count) {
    chunk_column_size = subdivide.x + 1;
    chunk_row_size = subdivide.y + 1;
    chunk_vertex_count = chunk_column_size * chunk_row_size;
    chunk_size = chunk_vertex_count * sizeof(TerrainVertex);

    chunk_buffer = VertexBuffer(nullptr, chunk_count * chunk_size);
    chunk_offset_buffer = Buffer<GL_SHADER_STORAGE_BUFFER>(nullptr, chunk_count * sizeof(glm::vec<2, u16>));
    command_buffer = Buffer<GL_DRAW_INDIRECT_BUFFER>(nullptr, chunk_count * sizeof(DrawElementsIndirectCommand));

    chunks.resize(chunk_count / 8);
    chunk_offsets.resize(chunk_count);

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.05f);
}

std::array<int, 4> HeightMapChunkManager::get_neighboring_vertices(int x, int y) {
    std::array<int, 4> indices = {
        (x + 1) * int(chunk_column_size) + y,
        (x * int(chunk_column_size)) + y - 1, 
        (x - 1) * int(chunk_column_size) + y,
        (x * int(chunk_column_size)) + y + 1
    };

    if (x == chunk_column_size - 1) indices[0] = -1;
    if (y == 0)                     indices[1] = -1;
    if (x == 0)                     indices[2] = -1;
    if (y == chunk_row_size - 1)    indices[3] = -1;

    return indices;
}

glm::vec3 edge_to_direction(int neighbor_vertex_i, float neighbor_height, float current_height) {
    glm::vec3 relative_position;
    switch (neighbor_vertex_i) {
    case 0:
        relative_position = glm::vec3( 1.0f, 0.0f,  0.0f);
        break;
    case 1:
        relative_position = glm::vec3( 0.0f, 0.0f, -1.0f);
        break;
    case 2:
        relative_position = glm::vec3(-1.0f, 0.0f,  0.0f);
        break;
    case 3:
        relative_position = glm::vec3( 0.0f, 0.0f,  1.0f);
        break;
    }
    
    relative_position.y = current_height - neighbor_height;

    return glm::normalize(relative_position);
}

HeightMapChunkManager::ChunkMesh HeightMapChunkManager::generate_chunk(glm::vec2 size, glm::uvec2 subdivide, glm::vec<2, i16> position) {
    std::vector<TerrainVertex> vertices;
    std::vector<u8> indices;

    vertices.reserve(chunk_vertex_count);
    indices.reserve(chunk_index_count);

    constexpr float PI = 3.14159265359f;

    for (int x = 0; x < chunk_column_size; x++) {
        for (int y = 0; y < chunk_column_size; y++) {
            vertices.emplace_back(TerrainVertex {
                noise.GetNoise(
                    float(x + position.x * (chunk_column_size - 1)),
                    float(y + position.y * (chunk_row_size - 1))
                ),
                0
            });
        }
    }

    for (int x = 0; x < chunk_column_size; x++) {
        for (int y = 0; y < chunk_row_size; y++) {
            TerrainVertex& current_vertex = vertices[(x * chunk_column_size) + y];

            std::array<int, 4> neighboring_vertices = get_neighboring_vertices(x, y);

            int skipped_faces = 0;

            glm::vec3 sum(0.0f);
            for (int i = 0; i < neighboring_vertices.size(); i++) {
                int next = (i + 1) % neighboring_vertices.size();

                if (neighboring_vertices[i] == -1 || neighboring_vertices[next] == -1) {
                    skipped_faces++;
                    continue;
                }

                glm::vec3 dir1 = edge_to_direction(next, vertices[neighboring_vertices[next]].height, current_vertex.height);
                glm::vec3 dir2 = edge_to_direction(i,    vertices[neighboring_vertices[i   ]].height, current_vertex.height);
                glm::vec3 normal = glm::normalize(glm::cross(dir1, dir2));

                sum += normal;
            }

            glm::vec3 normal = glm::normalize(sum * (1.0f / (neighboring_vertices.size() - skipped_faces)));

            const u32 packed_data = glm::packSnorm2x16(glm::vec2(normal.x, normal.z));

            current_vertex.normal_components = packed_data;
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

std::vector<u16> HeightMapChunkManager::get_used_chunk_indeces() const {
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

void HeightMapChunkManager::add_chunk(glm::vec<2, i16> chunk_pos) {
    size_t idx = reserve_chunk();

    ChunkMesh mesh = generate_chunk(glm::vec2(10.0f), glm::uvec2(10), chunk_pos);

    chunk_offsets[idx] = chunk_pos;
    chunk_buffer.set_data(mesh.vertices.data(), chunk_size, idx * chunk_size);
}

bool should_draw_chunk(glm::vec2 chunk_world_offset, glm::vec2 chunk_size, const Camera3D::Frustum& view_frustum) {
    glm::vec2 min = chunk_world_offset;
    glm::vec2 max = chunk_world_offset + chunk_size;

    std::array<glm::vec3, 4> corners = {
        glm::vec3(min.x, 0.0f, min.y),
        glm::vec3(min.x, 0.0f, max.y),
        glm::vec3(max.x, 0.0f, min.y),
        glm::vec3(max.x, 0.0f, max.y),
        /* glm::vec3(min.x, -10.0f, min.y),
        glm::vec3(min.x, -10.0f, max.y),
        glm::vec3(max.x, -10.0f, min.y),
        glm::vec3(max.x, -10.0f, max.y) */
    };

    auto plane_test = [&](const Camera3D::Plane& plane) {
        // If all corners are outside this plane, the chunk is culled
        for (auto& c : corners) {
            float dist = glm::dot(plane.normal, c) - plane.distance;
            if (dist >= 0.0f) {
                return true; // at least one corner inside
            }
        }
        return false; // all outside
    };

    if (!plane_test(view_frustum.left_face))  return false;
    if (!plane_test(view_frustum.right_face)) return false;
    if (!plane_test(view_frustum.near_face))  return false;
    if (!plane_test(view_frustum.far_face))   return false;

    return true;
}

size_t HeightMapChunkManager::generate_draw_commands(const Camera3D::Frustum& view_frustum) {
    std::vector<DrawElementsIndirectCommand> commands;
    commands.reserve(chunk_count);

    std::vector<u16> used_chunks = get_used_chunk_indeces();

    for (u16 chunk_idx : used_chunks) {
        glm::vec2 world_offset = glm::vec2(chunk_offsets[chunk_idx]) * glm::vec2(chunk_column_size, chunk_row_size);
        if (should_draw_chunk(world_offset, glm::vec2(chunk_column_size, chunk_row_size), view_frustum)) {
            commands.emplace_back(DrawElementsIndirectCommand {
                .count = chunk_index_count,
                .instance_count = 1,
                .first_index = 0,
                .base_vertex = chunk_idx * chunk_vertex_count,
                .base_instance = chunk_idx,
            });
        }
    }

    chunk_offset_buffer.set_data(chunk_offsets.data(), chunk_offsets.size() * sizeof(glm::vec<2, u16>));
    command_buffer.set_data(commands.data(), commands.size() * sizeof(DrawElementsIndirectCommand));
    return commands.size();
}

u16 HeightMapChunkManager::get_chunk_id_by_pos(glm::vec<2, i16> chunk_pos) const {
    std::vector<u16> used_indeces = get_used_chunk_indeces();
    for (u16 i : used_indeces) {
        if (chunk_offsets[i] == chunk_pos) {
            return i;
        }
    }
    return -1;
}

u16 HeightMapChunkManager::get_chunk_id_by_world_pos(glm::vec2 world_pos) const {
    glm::vec<2, i16> chunk_pos;
    chunk_pos.x = world_pos.x / chunk_column_size;
    chunk_pos.y = world_pos.y / chunk_row_size;
    return get_chunk_id_by_pos(chunk_pos); 
}

float HeightMapChunkManager::get_vertex_height_in_chunk_by_pos(u16 chunk_index, int x, int y) {
    glm::vec<2, u16> offset = chunk_offsets[chunk_index];
    return noise.GetNoise(
        float(x + offset.x * (chunk_column_size - 1)),
        float(y + offset.y * (chunk_row_size - 1))
    );
}

float HeightMapChunkManager::get_vertex_height_by_world_pos(glm::vec2 world_pos) {
    return noise.GetNoise(world_pos.x, world_pos.y);
}

void HeightMapChunkManager::bind() const {
    chunk_buffer.bind();
    chunk_offset_buffer.bind();
    gl_call(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunk_offset_buffer.get_id()));
    command_buffer.bind();
}
