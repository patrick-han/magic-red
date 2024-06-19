#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

enum class CameraMovementDirection 
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
	glm::vec3 m_position;
	glm::vec3 m_worldUp;
	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_localUp;
	float m_yaw;
	float m_pitch;
	float m_fov;
	bool m_bAllowMovement;
public:
	Camera(glm::vec3 _position, glm::vec3 _worldUp, glm::vec3 _front, float _yaw, float _pitch, float _fov, bool _allowMovement);

	void process_keyboard_input(CameraMovementDirection direction, float cameraSpeed);

	void process_mouse_movement(float xoffset, float yoffset, bool constrainPitch);

	void adjust_fov(float scrollOffset);

	glm::mat4 get_view_matrix();

	void freeze_camera();

	void unfreeze_camera();

private:
	void update_camera_vectors();
};