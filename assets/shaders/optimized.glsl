#shader vertex
#version 430 core
#extension GL_ARB_shader_draw_parameters : require

layout(std430, binding = 0) buffer chunk_offsets {
    uint offsets[];
};

layout(location = 0) in float a_height;
layout(location = 1) in uint a_packed_yaw_pitch;

uniform mat4 mvp;
uniform mat4 model;
uniform uvec2 chunk_size;
uniform vec2 quad_size;

out vec3 normal;
out vec3 frag_pos;

const float PI = 3.14159265359;

vec3 direction_from_yaw_pitch(float yaw, float pitch) {
    float cos_pitch = cos(pitch);
    return vec3(
        cos_pitch * cos(yaw),   // X
        sin(pitch),        // Y
        cos_pitch * sin(yaw)    // Z
    );
}

vec2 unpacki16x2(uint packed_data) {
    int x = int(packed_data << 16) >> 16;
    int y = int(packed_data >> 16);
    return vec2(x, y);
}

void main() {
    uint vertex_index = uint(gl_VertexID) % (chunk_size.x * chunk_size.y);
    uint x = vertex_index / chunk_size.x;
    uint y = vertex_index % chunk_size.x;

    uint packed_chunk_offset = offsets[gl_DrawIDARB];
    vec2 chunk_offset = quad_size * vec2(chunk_size - uvec2(1, 1)) * unpacki16x2(packed_chunk_offset);

    vec3 world_position = vec3(x * quad_size.x, a_height, y * quad_size.y);
    world_position.x += chunk_offset.x;
    world_position.z += chunk_offset.y;
    gl_Position = mvp * vec4(world_position, 1.0);

    vec2 components = unpackSnorm2x16(a_packed_yaw_pitch);
    normal = normalize(vec3(components.x, sqrt(max(0.0, 1.0 - components.x*components.x - components.y*components.y)), components.y));

    frag_pos = vec3(model * vec4(world_position, 1.0));
}

#shader fragment
#version 430 core

layout(location = 0) out vec4 frag_color;

in vec3 normal;
in vec3 frag_pos;

struct Material {
    vec3 color;
    vec3 specular;
    float shininess;
};

struct Sun {
    vec3 direction;
    vec3 diffuse;
};

uniform Material material;
uniform Sun sun;
uniform vec3 view_pos;

void main() {
    // Ambient light
    vec3 ambient = 0.2 * material.color * sun.diffuse;

    // Diffuse light
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(-sun.direction);

    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * sun.diffuse;

    // Specular light
    float specular_strength = 0.5;

    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = sun.diffuse * (specular_strength * spec);
    
    vec3 result = ambient + diffuse * material.color + specular;
    frag_color = vec4(result, 1.0);
}
