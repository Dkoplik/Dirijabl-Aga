#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class Camera {
public:
    // Конструктор
    Camera(glm::vec3 position = glm::vec3(0.0f, 10.0f, 20.0f),
           glm::vec3 front = glm::vec3(0.0f, -0.3f, -1.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

    // Мировые координаты в координаты камеры
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

    // Повороты камеры (pitch и yaw - в градусах)
    void rotatePitch(float degrees);   // Вверх/вниз (по Y)
    void rotateYaw(float degrees);     // Влево/вправо (по Z)

    // Параметры
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    float fov = 45.0f; // угол обзора
    float pitch = 0.0f; // угол наклона вверх/вниз
    float yaw = -90.0f; // угол поворота влево/вправо  

private:
    void updateCameraVectors();
};

#endif 
