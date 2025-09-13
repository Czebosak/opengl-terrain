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
        glm::vec3 position;
        u16 yaw;
        u16 pitch;
    };

    struct TerrainMesh {
        std::vector<TerrainVertex> vertices;
        std::vector<u32> indices;
    };

    TerrainMesh generate_plane(glm::vec2 size, glm::vec2 subdivide = glm::ivec2(0));
public:
    HeightMapTerrain(glm::vec2 size, glm::vec2 subdivide);
};
