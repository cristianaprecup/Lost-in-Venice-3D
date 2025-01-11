#include "camera.h"

Camera::Camera(glm::vec3 cameraPosition)
{
    this->cameraPosition = cameraPosition;
    this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f); 
    this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);             
    this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
}

Camera::Camera()
{
    this->cameraPosition = glm::vec3(0.0f, -10.0f, 0.0f);
    this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -0.5f);
    this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
}

Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraViewDirection, glm::vec3 cameraUp)
{
    this->cameraPosition = cameraPosition;
    this->cameraViewDirection = glm::normalize(cameraViewDirection);
    this->cameraUp = glm::normalize(cameraUp);
    this->cameraRight = glm::normalize(glm::cross(cameraViewDirection, cameraUp));
}

Camera::~Camera() {}

void Camera::keyboardMoveFront(float cameraSpeed)
{
    cameraPosition += cameraViewDirection * cameraSpeed;
}

void Camera::keyboardMoveBack(float cameraSpeed)
{
    cameraPosition -= cameraViewDirection * cameraSpeed; 
}

void Camera::keyboardMoveLeft(float cameraSpeed)
{
    cameraPosition -= cameraRight * cameraSpeed; 
}

void Camera::keyboardMoveRight(float cameraSpeed)
{
    cameraPosition += cameraRight * cameraSpeed; 
}

void Camera::keyboardMoveUp(float cameraSpeed)
{
    cameraPosition += glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed; 
}

void Camera::keyboardMoveDown(float cameraSpeed)
{
    cameraPosition -= glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed; 
}

void Camera::rotateOy(float angle)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    cameraViewDirection = glm::normalize(glm::vec3(rotation * glm::vec4(cameraViewDirection, 0.0f)));

    
    cameraRight = glm::normalize(glm::cross(cameraViewDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); 
}

void Camera::rotateOx(float angle) {
    // Implementation of the function
}


glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(cameraPosition, cameraPosition + cameraViewDirection, cameraUp);
}

glm::vec3 Camera::getCameraPosition()
{
    return cameraPosition;
}

glm::vec3 Camera::getCameraViewDirection()
{
    return cameraViewDirection;
}

glm::vec3 Camera::getCameraUp()
{
    return cameraUp;
}
