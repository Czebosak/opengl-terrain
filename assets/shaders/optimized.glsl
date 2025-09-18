#shader vertex
#version 430 core
#extension GL_ARB_shader_draw_parameters : require

layout(std430, binding = 0) buffer chunk_offsets {
    uint offsets[];
};

layout(location = 0) in float a_height;
layout(location = 1) in uint a_packed_yaw_pitch;

uniform mat4 mvp;
uniform uvec2 chunk_size;
uniform vec2 quad_size;

const float PI = 3.14159265359;

vec3 direction_from_yaw_pitch(float yaw, float pitch) {
    float cos_pitch = cos(pitch);
    return vec3(
        cos_pitch * sin(yaw),
        sin(pitch),
        cos_pitch * cos(yaw)
    );
}

vec2 unpack_yaw_and_pitch(uint packed_data) {
    return vec2(
        (packed_data & 0xFFFFu) / 65535.0 * 2 * PI, // Convert to 0-360 degree angle
        (((packed_data >> 16) & 0xFFFFu) / 65535.0 * PI) + PI // Convert to 180-360 degree angle
    );
}

void main() {
    uint vertex_index = uint(gl_VertexID) % (chunk_size.x * chunk_size.y); // - uint(gl_BaseVertex);
    uint x = vertex_index / chunk_size.x;
    uint y = vertex_index % chunk_size.x;

    //vec2 chunk_offset = chunk_offsets[gl_DrawID]; gl_DrawIDARB
    vec2 chunk_offset = quad_size * vec2(chunk_size - uvec2(1, 1));

    vec3 world_position = vec3(x * quad_size.x, a_height, y * quad_size.y);
    world_position.x += chunk_offset.x * (offsets[gl_DrawIDARB] & 0xFFFFu);
    world_position.z += chunk_offset.y * (offsets[gl_DrawIDARB] >> 16);
    gl_Position = mvp * vec4(world_position, 1.0);
    /* vec2 yaw_and_pitch = unpack_yaw_and_pitch(a_packed_yaw_pitch);
    vec3 normal = direction_from_yaw_pitch(yaw_and_pitch.x, yaw_and_pitch.y); */
}

#shader fragment
#version 430 core

layout(location = 0) out vec4 frag_color;

uniform vec4 color;

void main() {
    frag_color = color;
}
