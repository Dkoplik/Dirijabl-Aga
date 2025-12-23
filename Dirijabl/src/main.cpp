#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <vector>

#include "camera.h"
#include "model.h"
#include "shader.h"

// Models
Model* tableModel = nullptr;
Model* chairModel = nullptr;
Model* vaseModel = nullptr;

Shader* shader = nullptr;
Camera* camera = nullptr;

// Directional light parameters
glm::vec3 dirLightDirection = glm::vec3(-0.3f, -1.0f, -0.2f);
glm::vec3 dirLightAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
glm::vec3 dirLightDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
glm::vec3 dirLightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

GLuint loadTexture(const std::string& filename) {
  sf::Image image;
  if (image.loadFromFile(filename)) {
    image.flipVertically();
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    sf::Vector2u size = image.getSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Texture loaded: " << filename << std::endl;
    return texture;
  } else {
    std::cerr << "Couldn't load texture: " << filename << std::endl;
    return 0;
  }
}

void initGL() {
  glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  std::cout << " OpenGL initialized" << std::endl;
  std::cout << " Version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << " Vendor: " << glGetString(GL_VENDOR) << std::endl;
}

void initResources() {
  // Create models
  tableModel = new Model("models/table.obj");
  tableModel->loadTexture("textures/table.png");

  // Create multiple table instances
  ModelInstance* table1 = tableModel->createInstance();
  table1->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
  table1->setScale(glm::vec3(2.0f, 2.0f, 2.0f));

  ModelInstance* table2 = tableModel->createInstance();
  table2->setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
  table2->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), 45.0f);
  table2->setScale(glm::vec3(1.5f, 1.5f, 1.5f));

  chairModel = new Model("models/chair.obj");
  chairModel->loadTexture("textures/chair.png");

  // Create multiple chair instances
  ModelInstance* chair1 = chairModel->createInstance();
  chair1->setPosition(glm::vec3(2.0f, 0.0f, 2.0f));
  chair1->setScale(glm::vec3(0.04f, 0.04f, 0.04f));
  chair1->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), -50.0f);

  ModelInstance* chair2 = chairModel->createInstance();
  chair2->setPosition(glm::vec3(-2.0f, 0.0f, 2.0f));
  chair2->setScale(glm::vec3(0.04f, 0.04f, 0.04f));
  chair2->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), 50.0f);

  vaseModel = new Model("models/vase.obj");
  vaseModel->loadTexture("textures/vase.png");

  // Create vase instance
  ModelInstance* vase = vaseModel->createInstance();
  vase->setPosition(glm::vec3(-1.0f, 2.83f, 0.0f));
  vase->setScale(glm::vec3(0.15f, 0.15f, 0.15f));

  shader = new Shader();
  std::cout << "Shader initialized" << std::endl;
}

void render(float width, float height) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!shader) return;

  shader->use();

  glm::mat4 view = camera->getViewMatrix();
  glm::mat4 projection = camera->getProjectionMatrix(width / height);

  // Set view and projection matrices
  shader->setMat4("view", view);
  shader->setMat4("projection", projection);

  // Set view position
  shader->setVec3("viewPos", camera->position);

  // Set directional light properties
  shader->setVec3("dirLight.direction", dirLightDirection);
  shader->setVec3("dirLight.ambient", dirLightAmbient);
  shader->setVec3("dirLight.diffuse", dirLightDiffuse);
  shader->setVec3("dirLight.specular", dirLightSpecular);

  // Draw all instances of each model
  if (tableModel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tableModel->texture);
    shader->setInt("textureSampler", 0);
    tableModel->drawAllInstances();
  }

  if (chairModel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, chairModel->texture);
    shader->setInt("textureSampler", 0);
    chairModel->drawAllInstances();
  }

  if (vaseModel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vaseModel->texture);
    shader->setInt("textureSampler", 0);
    vaseModel->drawAllInstances();
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void handleInput(float deltaTime) {
  float moveSpeed = 5.0f * deltaTime;
  float rotateSpeed = 50.0f * deltaTime;

  // Camera movement
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    camera->moveForward(moveSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    camera->moveBackward(moveSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    camera->moveLeft(moveSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    camera->moveRight(moveSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
    camera->moveUp(moveSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
    camera->moveDown(moveSpeed);
  }

  // Camera rotation
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
    camera->rotatePitch(rotateSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
    camera->rotatePitch(-rotateSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
    camera->rotateYaw(-rotateSpeed);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
    camera->rotateYaw(rotateSpeed);
  }

  // Reset camera with R
  static bool rKeyPressed = false;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
    if (!rKeyPressed) {
      delete camera;
      camera = new Camera(glm::vec3(0.0f, 5.0f, 20.0f));
      rKeyPressed = true;
    }
  } else {
    rKeyPressed = false;
  }
}

int main() {
  setlocale(LC_ALL, "ru.UTF-8");
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 3;
  settings.attributeFlags = sf::ContextSettings::Core;

  sf::RenderWindow window(sf::VideoMode(1200, 800),
                          "Model Viewer with Instanced Rendering",
                          sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);
  window.setActive(true);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "GLEW initialization error" << std::endl;
    return -1;
  }

  std::cout << "\n=== MODEL VIEWER WITH INSTANCED RENDERING ===" << std::endl;
  std::cout << std::endl;

  initGL();
  initResources();

  camera =
      new Camera(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                 glm::vec3(0.0f, 1.0f, 0.0f));

  std::cout << std::endl;
  std::cout << "CONTROLS:" << std::endl;
  std::cout << " W/A/S/D      - movement" << std::endl;
  std::cout << " SPACE/CTRL   - up/down" << std::endl;
  std::cout << " Arrows       - camera rotation" << std::endl;
  std::cout << " R            - reset camera" << std::endl;
  std::cout << " ESC          - exit" << std::endl;
  std::cout << std::endl;
  std::cout << "Multiple instances of each model are rendered using instanced "
               "rendering"
            << std::endl;

  sf::Clock clock;
  bool running = true;

  while (running && window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        running = false;
      }

      if (event.type == sf::Event::Resized) {
        glViewport(0, 0, event.size.width, event.size.height);
      }
    }

    float deltaTime = clock.restart().asSeconds();
    handleInput(deltaTime);
    render(window.getSize().x, window.getSize().y);
    window.display();
  }

  // Cleanup
  delete shader;
  delete camera;
  delete tableModel;
  delete chairModel;
  delete vaseModel;

  window.close();
  std::cout << "Program finished" << std::endl;

  return 0;
}