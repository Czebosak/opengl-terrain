#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <limits>

#include <renderer.hpp>
#include <camera3d.hpp>
#include <mesh3d.hpp>
#include <texture.hpp>

#include <player_camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::vec2 mouse_delta(0.0f);

void mouse_move_callback(GLFWwindow* window, double x_pos, double y_pos) {
    static double last_x = x_pos;
    static double last_y = y_pos;
    mouse_delta.x += x_pos - last_x;
    mouse_delta.y += last_y - y_pos;
    last_x = x_pos;
    last_y = y_pos;
}

GLFWwindow* setup_window_and_context(u32 width, u32 height, const char* title) {
    /* Initialize the library */
    if (!glfwInit()) {
        fprintf(stderr, "glfw failed to initialize ヾ(ﾟдﾟ)ﾉ゛");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        fprintf(stderr, "Window failed to initialize ヾ(ﾟдﾟ)ﾉ゛");
        glfwTerminate();
        return nullptr;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "GLEW %s\nOPENGL %s\n", glewGetString(GLEW_VERSION), glGetString(GL_VERSION));
    
    // Setup blending
    gl_call(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    gl_call(glEnable(GL_BLEND));
    gl_call(glEnable(GL_DEPTH_TEST));
    gl_call(glEnable(GL_MULTISAMPLE));

    return window;
}

int main(void) {
    GLFWwindow* window = setup_window_and_context(1920, 1080, "Example");
    if (!window) {
        return -1;
    }

    glfwSetCursorPosCallback(window, mouse_move_callback);  

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    PlayerCamera player_camera(
        1920, 1080,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::perspective(glm::radians(45.0f), 1920.0f/1080.0f, 0.1f, 1000.0f)
    );

    std::vector<Vertex3D> vertices = {
        // Front face (normal +Z)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}}, // 0
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}}, // 1
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}}, // 2
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}}, // 3

        // Back face (normal -Z)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}}, // 4
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}}, // 5
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}, // 6
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}}, // 7

        // Left face (normal -X)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // 8
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // 9
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // 10
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // 11

        // Right face (normal +X)
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}}, // 12
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}}, // 13
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}}, // 14
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}, // 15

        // Top face (normal +Y)
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}}, // 16
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}}, // 17
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}}, // 18
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}, // 19

        // Bottom face (normal -Y)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}}, // 20
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}}, // 21
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}}, // 22
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, // 23
    };

    std::vector<u32> indices = {
        // Front
        0, 1, 2,  2, 3, 0,
        // Back
        4, 5, 6,  6, 7, 4,
        // Left
        8, 9, 10, 10, 11, 8,
        // Right
        12, 13, 14, 14, 15, 12,
        // Top
        16, 17, 18, 18, 19, 16,
        // Bottom
        20, 21, 22, 22, 23, 20
    };

    glm::mat4 cube_transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
    Texture cube_texture("/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/textures/cube.png");
    Shader cube_shader("/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaded.glsl");
    cube_shader.bind();
    cube_shader.set_uniform_1i("u_material.diffuse", 0);
    cube_shader.set_uniform_v3("u_material.specular", 1.0f, 1.0f, 1.0f);
    cube_shader.set_uniform_1f("u_material.shininess", 32.0f);
    cube_shader.set_uniform_mat4f("u_model", cube_transform);
    Mesh3D cube(std::move(vertices), std::move(indices));

    cube_shader.set_uniform_v3("u_sun.direction", -1.0f, -1.0f, 0.5f);
    cube_shader.set_uniform_v3("u_sun.diffuse", 1.0f, 1.0f, 0.9f);

    glm::mat4 vp;
    double delta, last_frame = 0.0f;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        gl_call(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        gl_call(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        double current_frame = glfwGetTime();
        delta = current_frame - last_frame;
        last_frame = current_frame;

        player_camera.update(delta, window, mouse_delta);

        vp = player_camera.matrix();

        cube_texture.bind();
        cube_shader.bind();
        cube_shader.set_mvp(vp * cube_transform);
        cube_shader.set_uniform_v3("u_view_pos", player_camera.position.x, player_camera.position.y, player_camera.position.z);
        cube.bind();
        gl_call(glDrawElements(GL_TRIANGLES, cube.get_index_buffer().get_count(), GL_UNSIGNED_INT, nullptr));

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
