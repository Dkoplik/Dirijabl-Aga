#pragma once

#include <glm/glm.hpp>
#include <vector>

// Структура для описания орбитального объекта
struct CelestialBody {
    float orbitRadius;
    float orbitSpeed;       // градусы/кадр
    float rotationSpeed;    // градусы/кадр
    float scale;
    glm::vec3 orbitAxis;

    float currentOrbitAngle = 0.0f;
    float currentRotationAngle = 0.0f;
    glm::vec3 orbitCenter;

    glm::vec3 getOrbitPosition() const;
    glm::mat4 getModelMatrix() const;
    void update(float deltaTime = 1.0f);
};

// Система управления планетами
class SolarSystem {
public:
    SolarSystem();

    void addBody(const CelestialBody& body);

    void update(float deltaTime = 1.0f);

    const std::vector<CelestialBody>& getBodies() const { return bodies; }
    std::vector<CelestialBody>& getBodies() { return bodies; }

    size_t getBodyCount() const { return bodies.size(); }

    std::vector<glm::mat4> getModelMatrices() const;

private:
    std::vector<CelestialBody> bodies;
};