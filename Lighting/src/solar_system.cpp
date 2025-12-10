#include "solar_system.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// ==============================
// CelestialBody
// ==============================
glm::vec3 CelestialBody::getOrbitPosition() const {
    float radians = glm::radians(currentOrbitAngle);
    return orbitCenter + glm::vec3(
        orbitRadius * std::cos(radians),
        0.0f,
        orbitRadius * std::sin(radians)
    );
}

glm::mat4 CelestialBody::getModelMatrix() const {
    glm::vec3 pos = getOrbitPosition();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, glm::radians(currentRotationAngle),
        glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale));

    return model;
}

void CelestialBody::update(float deltaTime) {
    currentOrbitAngle += orbitSpeed * deltaTime;
    if (currentOrbitAngle >= 360.0f) {
        currentOrbitAngle -= 360.0f;
    }

    currentRotationAngle += rotationSpeed * deltaTime;
    if (currentRotationAngle >= 360.0f) {
        currentRotationAngle -= 360.0f;
    }
}

// ==============================
// SolarSystem
// ==============================
SolarSystem::SolarSystem() {
}

void SolarSystem::addBody(const CelestialBody& body) {
    bodies.push_back(body);
}

void SolarSystem::update(float deltaTime) {
    for (auto& body : bodies) {
        body.update(deltaTime);
    }
}

std::vector<glm::mat4> SolarSystem::getModelMatrices() const {
    std::vector<glm::mat4> matrices;
    matrices.reserve(bodies.size());

    for (const auto& body : bodies) {
        matrices.push_back(body.getModelMatrix());
    }

    return matrices;
}