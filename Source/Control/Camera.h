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
	glm::vec3 position;
	glm::vec3 worldUp;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;
	float yaw;
	float pitch;
	float fov;
	bool allowMovement;
public:
	Camera(glm::vec3 _position, glm::vec3 _worldUp, glm::vec3 _front, float _yaw, float _pitch, float _fov, bool _allowMovement) :
		position(_position),
		worldUp(glm::normalize(_worldUp)),
		front(glm::normalize(_front)),
		right(glm::normalize(glm::cross(front, up))),
		up(glm::normalize(glm::cross(right, front))),
		yaw(_yaw),
		pitch(_pitch),
		fov(_fov),
		allowMovement(_allowMovement)
	{
	}

	void process_keyboard_input(CameraMovementDirection direction, float cameraSpeed) 
	{
		if(allowMovement) {
			if (direction == CameraMovementDirection::FORWARD)
			{
				position += cameraSpeed * front;
			}
			if (direction == CameraMovementDirection::BACKWARD)
			{
				position -= cameraSpeed * front;
			}
			if (direction == CameraMovementDirection::LEFT)
			{
				position -= glm::normalize(glm::cross(front, up)) * cameraSpeed;
			}
			if (direction == CameraMovementDirection::RIGHT)
			{
				position += glm::normalize(glm::cross(front, up)) * cameraSpeed;
			}
			if (direction == CameraMovementDirection::UP)
			{
				position += cameraSpeed * worldUp;
			}
			if (direction == CameraMovementDirection::DOWN)
			{
				position += cameraSpeed * -worldUp;
			}
		}
	}

	void process_mouse_movement(float xoffset, float yoffset, bool constrainPitch) 
	{
		if(allowMovement) 
		{
			// Update camera pitch and yaw based on mouse offsets calculated in the mouse callback
			yaw += xoffset;
			pitch += yoffset;

			if (constrainPitch) 
			{
				if (pitch > 89.0f) // Prevent weird flipping when looking at exactly 90 degrees (messes with lookat)
				{
					pitch = 89.0f;
				}
				if (pitch < -89.0f)
				{
					pitch = -89.0f;
				}
			}
			update_camera_vectors();
		}
	}

	void adjust_fov(float scrollOffset) 
	{
		fov -= (float)scrollOffset;
		if (fov < 1.0f)
		{
			fov = 1.0f;
		}

		if (fov > 45.0f)
		{
			fov = 45.0f;
		}
	}

	glm::mat4 get_view_matrix() 
	{
		// Return the view matrix which is just at lookAt matrix calculated from the cameras 3 main directional vectors
		glm::mat4 view;
		view = glm::lookAt(position, position + front, up);
		return view;
	}

	void freezeCamera() 
	{
		allowMovement = false;
	}

	void unfreezeCamera() 
	{
		allowMovement = true;
	}

private:
	void update_camera_vectors() 
	{
		// Update the direction the camera is looking at based on the camera yaw and pitch
		glm::vec3 direction; // Vector actually points towards camera from the looking position
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(direction);
		// also re-calculate the right and up vector
		right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, front));
	}
};