#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <limits>

#include <renderer.hpp>
#include <camera3d.hpp>
#include <mesh3d.hpp>
#include <texture.hpp>

#include <heightmap_terrain.hpp>
#include <player_camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/* #include <miniaudio.h> */

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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
    gl_call(glEnable(GL_CULL_FACE));
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    return window;
}

int main(void) {
    GLFWwindow* window = setup_window_and_context(1920, 1080, "Example");
    if (!window) {
        return -1;
    }
    
    /* ma_result result;
    ma_engine engine;
    
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        return -1;
    } */

    glfwSetCursorPosCallback(window, mouse_move_callback);  

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    PlayerCamera player_camera(
        1920, 1080,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::perspective(glm::radians(45.0f), 1920.0f/1080.0f, 0.1f, 1000.0f),
        glm::radians(45.0f),
        0.1f,
        1000.0f
    );

    glm::mat4 cube_transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
    Texture cube_texture("/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/textures/cube.png");
    Shader cube_shader("/home/czebosak/Development/cpp/graphics/opengl/terrain/assets/shaders/shaded.glsl");
    cube_shader.bind();
    cube_shader.set_uniform_1i("u_material.diffuse", 0);
    //cube_shader.set_uniform_v3("u_material.specular", 1.0f, 1.0f, 1.0f);
    cube_shader.set_uniform_1f("u_material.shininess", 32.0f);
    cube_shader.set_uniform_mat4f("u_model", cube_transform);
    Mesh3D cube = Mesh3D::cube();

    cube_shader.set_uniform_v3("u_sun.direction", -1.0f, -1.0f, 0.5f);
    cube_shader.set_uniform_v3("u_sun.diffuse", 1.0f, 1.0f, 0.9f);

    HeightMapTerrain terrain(glm::vec2(10.0), glm::uvec2(10));

    Shader& terrain_shader = terrain.get_shader();

    terrain_shader.set_uniform_1f("flashlight.cut_off", glm::cos(glm::radians(7.0f)));
    terrain_shader.set_uniform_1f("flashlight.outer_cut_off", glm::cos(glm::radians(12.5f)));

    glm::mat4 vp;
    Camera3D::Frustum view_frustum;
    double delta, last_frame = 0.0f;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        gl_call(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        gl_call(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        double current_frame = glfwGetTime();
        delta = current_frame - last_frame;
        last_frame = current_frame;

        {
            float height = terrain.get_vertex_height_by_world_pos(glm::vec2(player_camera.transform.translation.x, player_camera.transform.translation.z));
            player_camera.transform.translation.y = height + 1.0f;
        }

        player_camera.update(delta, window, mouse_delta);

        vp = player_camera.get_matrix();
        view_frustum = player_camera.create_frustum();

        {
            glm::vec3 camera_forward = player_camera.transform.get_forward();
            terrain_shader.set_uniform_v3("flashlight.position", player_camera.transform.translation.x, player_camera.transform.translation.y, player_camera.transform.translation.z);
            terrain_shader.set_uniform_v3("flashlight.direction", camera_forward.x, camera_forward.y, camera_forward.z);
        }

        cube_texture.bind();
        cube_shader.bind();
        cube_shader.set_mvp(vp * cube_transform);
        cube_shader.set_uniform_v3("u_view_pos", player_camera.transform.translation.x, player_camera.transform.translation.y, player_camera.transform.translation.z);
        cube.bind();
        gl_call(glDrawElements(GL_TRIANGLES, cube.get_index_buffer().get_count(), GL_UNSIGNED_INT, nullptr));

        terrain.draw(vp, view_frustum);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    /* ma_engine_uninit(&engine); */

    glfwTerminate();
    return 0;
}
