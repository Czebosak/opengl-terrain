#pragma once
#include <GLFW/glfw3.h>

#include <utils.hpp>
#include <camera3d.hpp>

#include <glm/glm.hpp>

class PlayerCamera : public Camera3D {
public:
    PlayerCamera(u32 width, u32 height, glm::vec3 position, glm::mat4 projection, float fov_y, float near, float far) : Camera3D(width, height, position, projection, fov_y, near, far) {}

    void update(double delta, GLFWwindow* window, glm::vec2& mouse_delta);
};