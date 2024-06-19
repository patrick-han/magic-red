#include "Camera.h"


Camera::Camera(glm::vec3 _position, glm::vec3 _worldUp, glm::vec3 _front, float _yaw, float _pitch, float _fov, bool _allowMovement) :
    m_position(_position),
    m_worldUp(glm::normalize(_worldUp)),
    m_forward(glm::normalize(_front)),
    m_right(glm::normalize(glm::cross(m_forward, m_localUp))),
    m_localUp(glm::normalize(glm::cross(m_right, m_forward))),
    m_yaw(_yaw),
    m_pitch(_pitch),
    m_fov(_fov),
    m_bAllowMovement(_allowMovement)
{
}

void Camera::process_keyboard_input(CameraMovementDirection direction, float cameraSpeed) 
{
    if(m_bAllowMovement) {
        if (direction == CameraMovementDirection::FORWARD)
        {
            m_position += cameraSpeed * m_forward;
        }
        if (direction == CameraMovementDirection::BACKWARD)
        {
            m_position -= cameraSpeed * m_forward;
        }
        if (direction == CameraMovementDirection::LEFT)
        {
            m_position -= glm::normalize(glm::cross(m_forward, m_localUp)) * cameraSpeed;
        }
        if (direction == CameraMovementDirection::RIGHT)
        {
            m_position += glm::normalize(glm::cross(m_forward, m_localUp)) * cameraSpeed;
        }
        if (direction == CameraMovementDirection::UP)
        {
            m_position += cameraSpeed * m_worldUp;
        }
        if (direction == CameraMovementDirection::DOWN)
        {
            m_position += cameraSpeed * -m_worldUp;
        }
    }
}

void Camera::process_mouse_movement(float xoffset, float yoffset, bool constrainPitch)
{
    if(m_bAllowMovement) 
    {
        // Update camera pitch and yaw based on mouse offsets calculated in the mouse callback
        m_yaw += xoffset;
        m_pitch += yoffset;

        if (constrainPitch) 
        {
            if (m_pitch > 89.0f) // Prevent weird flipping when looking at exactly 90 degrees (messes with lookat)
            {
                m_pitch = 89.0f;
            }
            if (m_pitch < -89.0f)
            {
                m_pitch = -89.0f;
            }
        }
        update_camera_vectors();
    }
}

void Camera::adjust_fov(float scrollOffset) 
{
    m_fov -= (float)scrollOffset;
    if (m_fov < 1.0f)
    {
        m_fov = 1.0f;
    }

    if (m_fov > 45.0f)
    {
        m_fov = 45.0f;
    }
}

glm::mat4 Camera::get_view_matrix() 
{
    // Return the view matrix which is just at lookAt matrix calculated from the cameras 3 main directional vectors
    glm::mat4 view;
    view = glm::lookAt(m_position, m_position + m_forward, m_localUp);
    return view;
}

void Camera::freeze_camera() 
{
    m_bAllowMovement = false;
}

void Camera::unfreeze_camera() 
{
    m_bAllowMovement = true;
}


void Camera::update_camera_vectors()
{
    // Update the direction the camera is looking at based on the camera yaw and pitch
    glm::vec3 direction; // Vector actually points towards camera from the looking position
    direction.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    direction.y = sin(glm::radians(m_pitch));
    direction.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_forward = glm::normalize(direction);
    // also re-calculate the right and up vector
    m_right = glm::normalize(glm::cross(m_forward, m_worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    m_localUp = glm::normalize(glm::cross(m_right, m_forward));
}