#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {
 public:
  // Конструктор
  Camera(glm::vec3 position = glm::vec3(0.0f, 5.0f, 20.0f),
         glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

  // Получить матрицу вида
  glm::mat4 getViewMatrix() const;

  // Получить матрицу проекции
  glm::mat4 getProjectionMatrix(float aspect = 1.0f) const;

  // Управление камерой
  void moveForward(float distance);
  void moveBackward(float distance);
  void moveUp(float distance);
  void moveDown(float distance);
  void moveRight(float distance);
  void moveLeft(float distance);

  // Повороты камеры (в градусах)
  void rotatePitch(float degrees);  // Вверх/вниз
  void rotateYaw(float degrees);    // Влево/вправо

  // Параметры
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;

  float fov;    // Угол обзора
  float pitch;  // Угол наклона
  float yaw;    // Угол поворота

 private:
  void updateCameraVectors();
};

#endif