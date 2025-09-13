#shader vertex
#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_vertex_uv;

out vec2 uv;
out vec3 normal;
out vec3 frag_pos;

uniform mat4 u_model;
uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0f);
    uv = a_vertex_uv;
    normal = a_normal;
    frag_pos = vec3(u_model * vec4(a_position, 1.0));
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 uv;
in vec3 normal;
in vec3 frag_pos;

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
};

struct Sun {
    vec3 direction;
    vec3 diffuse;
};

uniform Material u_material;
uniform Sun u_sun;
uniform vec3 u_view_pos;

void main() {
    vec4 sample = texture(u_material.diffuse, uv);

    // Ambient light
    vec3 ambient = 0.2 * sample.rgb * u_sun.diffuse;

    // Diffuse light
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(-u_sun.direction);

    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * u_sun.diffuse;

    // Specular light
    float specular_strength = 0.5;

    vec3 view_dir = normalize(u_view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_material.shininess);
    vec3 specular = u_sun.diffuse * (specular_strength * spec);
    
    vec3 result = ambient + diffuse * sample.rgb + specular;
    color = vec4(result, sample.a);
}
