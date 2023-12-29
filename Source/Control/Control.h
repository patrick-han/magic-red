#pragma once

#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Common/Config.h"

// Camera
static glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
static Camera camera(cameraPos, cameraUp, cameraFront, -90.0f, 0.0f, 45.0f);
static float cameraSpeed = 0.0f;
static bool firstMouse = true;
static float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2; // Initial mouse positions

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

}

static void process_input(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cameraSpeed = 20.0f * deltaTime;
    } else {
        cameraSpeed = 10.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.process_keyboard_input(CameraMovementDirection::FORWARD, cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.process_keyboard_input(CameraMovementDirection::BACKWARD, cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.process_keyboard_input(CameraMovementDirection::LEFT, cameraSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.process_keyboard_input(CameraMovementDirection::RIGHT, cameraSpeed);
    }
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) // Input current mouse positions
{
	float xposf = static_cast<float>(xpos);
	float yposf = static_cast<float>(ypos);

	if (firstMouse) { // initially set to true, to prevent jump on loading in
		lastX = xposf;
		lastY = yposf;
		firstMouse = false;
	}

	float xoffset = xposf - lastX;
	float yoffset = lastY - yposf; // reversed since y-coordinates decrease pitching up
	lastX = xposf;
	lastY = yposf;

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	camera.process_mouse_movement(xoffset, yoffset, true);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	float yoffsetf = static_cast<float>(yoffset);
	camera.adjust_fov(yoffsetf);
}