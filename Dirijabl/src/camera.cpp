#include "camera.h"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up)
    : position(position),
      front(front),
      up(up),
      fov(45.0f),
      pitch(0.0f),
      yaw(-90.0f) {
  right = glm::normalize(glm::cross(front, up));
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
  return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
}

void Camera::moveForward(float distance) { position += front * distance; }

void Camera::moveBackward(float distance) { position -= front * distance; }

void Camera::moveUp(float distance) { position += up * distance; }

void Camera::moveDown(float distance) { position -= up * distance; }

void Camera::moveRight(float distance) { position += right * distance; }

void Camera::moveLeft(float distance) { position -= right * distance; }

void Camera::rotatePitch(float degrees) {
  pitch += degrees;

  if (pitch > 89.0f) pitch = 89.0f;
  if (pitch < -89.0f) pitch = -89.0f;

  updateCameraVectors();
}

void Camera::rotateYaw(float degrees) {
  yaw += degrees;
  updateCameraVectors();
}

void Camera::updateCameraVectors() {
  // Вычисляем направление взгляда из углов pitch и yaw
  glm::vec3 direction;
  direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  direction.y = std::sin(glm::radians(pitch));
  direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

  front = glm::normalize(direction);

  // Пересчитываем right и up векторы
  right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
  up = glm::normalize(glm::cross(right, front));
}