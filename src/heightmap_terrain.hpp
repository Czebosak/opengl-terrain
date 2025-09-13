#pragma once
#include <vertex_buffer.hpp>
#include <vertex_array.hpp>
#include <index_buffer.hpp>
#include <shader.hpp>

#include <vector>

class HeightMapTerrain {
private:
    VertexBuffer vertex_buffer;
    VertexArray vertex_array;
    IndexBuffer index_buffer;
    Shader shader;

    struct TerrainVertex {
        float height;
        u32 packed_yaw_and_pitch;
    };

    struct TerrainMesh {
        std::vector<TerrainVertex> vertices;
        std::vector<u8> indices;
    };

    TerrainMesh mesh;

    TerrainMesh generate_plane(glm::vec2 size, glm::vec2 subdivide = glm::ivec2(0));
public:
    HeightMapTerrain(glm::vec2 size, glm::vec2 subdivide);

    void bind() const;

    void draw(const glm::mat4& mvp);
};
