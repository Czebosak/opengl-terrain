#pragma once
#include <buffer.hpp>
#include <vertex_buffer.hpp>
#include <vertex_array.hpp>
#include <index_buffer.hpp>
#include <shader.hpp>

#include <vector>
#include <bitset>

#include <fastnoiselite.h>

#include <glm/glm.hpp>

class HeightMapChunkManager {
private:
    int chunk_count;
    u32 chunk_vertex_count;
    u32 chunk_index_count;
    u32 chunk_size;

    u32 chunk_column_size;
    u32 chunk_row_size;

    FastNoiseLite noise;    

    struct TerrainVertex {
        float height;
        u32 normal_components;
    };

    struct ChunkMesh {
        std::vector<TerrainVertex> vertices;
    };

    VertexBuffer chunk_buffer;
    Buffer<GL_SHADER_STORAGE_BUFFER> chunk_offset_buffer;
    Buffer<GL_DRAW_INDIRECT_BUFFER> command_buffer;
    std::vector<std::bitset<8>> chunks;
    std::vector<glm::vec<2, i16>> chunk_offsets;

    size_t reserve_chunk();
    std::vector<u16> get_used_chunk_indeces() const;

    std::array<int, 4> get_neighboring_vertices(int x, int y);

    //void add_chunk(glm::vec<2, u16> chunk_pos);
    ChunkMesh generate_chunk(glm::vec2 size, glm::uvec2 subdivide, glm::vec<2, i16> position);
public:
    HeightMapChunkManager() {}

    HeightMapChunkManager(int chunk_count, glm::uvec2 subdivide, u32 chunk_index_count);

    size_t generate_draw_commands();
    void add_chunk(glm::vec<2, i16> chunk_pos);

    u16 get_chunk_id_by_pos(glm::vec<2, i16> chunk_pos) const;
    u16 get_chunk_id_by_world_pos(glm::vec2 world_pos) const;

    void bind() const;

    inline const VertexBuffer& get_chunk_buffer() const { return chunk_buffer; }
};

class HeightMapTerrain {
private:
    VertexArray vertex_array;
    IndexBuffer index_buffer;
    Shader shader;

    glm::vec2 size;
    glm::uvec2 subdivide;

    HeightMapChunkManager chunk_manager;

    void generate_index_buffer();

    void bind() const;
public:
    HeightMapTerrain(glm::vec2 size, glm::uvec2 subdivide);

    float get_height(glm::vec2 position) const;

    void draw(const glm::mat4& mvp);
};
