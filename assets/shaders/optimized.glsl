#shader vertex
#version 330 core

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
    uint vertex_index = uint(gl_VertexID);
    uint x = vertex_index / chunk_size.x;
    uint y = vertex_index % chunk_size.x;

    gl_Position = mvp * vec4(x * quad_size.x, a_height, y * quad_size.y, 1.0);
    // vec2 yaw_and_pitch = unpack_yaw_and_pitch(a_packed_yaw_pitch);
    // direction_from_yaw_pitch();
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 frag_color;

uniform vec4 color;

void main() {
    frag_color = color;
}
