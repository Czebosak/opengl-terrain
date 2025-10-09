#include "player_camera.hpp"

const float CAMERA_SPEED = 10.0f;
const float CAMERA_VERTICAL_SPEED = 5.0f;

void PlayerCamera::update(double delta, GLFWwindow* window, glm::vec2& mouse_delta) {
    if (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f) {
        yaw -= mouse_delta.x * 0.001f;
        pitch += mouse_delta.y * 0.001f;
        pitch = glm::clamp(pitch, glm::radians(-80.0f), glm::radians(80.0f));
        mouse_delta = glm::vec2(0.0f);
    }

    glm::quat q_yaw   = glm::angleAxis(yaw, glm::vec3(0,1,0));
    glm::quat q_pitch = glm::angleAxis(pitch, glm::vec3(1,0,0));
    transform.rotation = glm::normalize(q_yaw * q_pitch);

    glm::vec3 camera_forward = transform.get_forward();
    glm::vec3 camera_right = transform.get_right();

    glm::vec3 input(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        input += camera_forward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        input -= camera_forward;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        input -= camera_right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        input += camera_right;
    }

    if (glm::length(input) > 0.0f) {
        transform.translation += glm::normalize(input) * static_cast<float>(delta) * CAMERA_SPEED;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        transform.translation.y += 1.0 * delta * CAMERA_VERTICAL_SPEED;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        transform.translation.y -= 1.0 * delta * CAMERA_VERTICAL_SPEED;
    }
}
