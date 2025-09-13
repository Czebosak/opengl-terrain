#shader vertex
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in float a_height;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0f);
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_color;

void main() {
    color = u_color;
}
