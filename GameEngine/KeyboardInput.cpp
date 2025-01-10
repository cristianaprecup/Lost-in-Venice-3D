#include "Camera/camera.h"
#include "Graphics/window.h"

void processKeyboardInput(Window& window, Camera& camera, float deltaTime) {
    float cameraSpeed = 20.0f * deltaTime;
    float rotationSpeed = glm::radians(90.0f * deltaTime * 20);

    if (window.isPressed(GLFW_KEY_W))
        camera.keyboardMoveFront(cameraSpeed);
    if (window.isPressed(GLFW_KEY_S))
        camera.keyboardMoveBack(cameraSpeed);
    if (window.isPressed(GLFW_KEY_A))
        camera.keyboardMoveLeft(cameraSpeed);
    if (window.isPressed(GLFW_KEY_D))
        camera.keyboardMoveRight(cameraSpeed);

    if (window.isPressed(GLFW_KEY_R))
        camera.keyboardMoveUp(cameraSpeed);
    if (window.isPressed(GLFW_KEY_F))
        camera.keyboardMoveDown(cameraSpeed);

    if (window.isPressed(GLFW_KEY_LEFT))
        camera.rotateOy(rotationSpeed);
    if (window.isPressed(GLFW_KEY_RIGHT))
        camera.rotateOy(-rotationSpeed);
}
