#pragma once
#include <GLFW/glfw3.h>

#include <utils.hpp>
#include <camera3d.hpp>

#include <glm/glm.hpp>

class PlayerCamera : public Camera3D {
public:
    float yaw;
    float pitch;

    PlayerCamera(u32 width, u32 height, Transform transform, glm::mat4 projection, float fov_y, float near, float far) : Camera3D(width, height, transform, projection, fov_y, near, far), yaw(glm::radians(0.0f)), pitch(0.0f) {}

    void update(double delta, GLFWwindow* window, glm::vec2& mouse_delta);
};