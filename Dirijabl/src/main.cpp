#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <chrono>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "camera.h"
#include "model.h"
#include "shader.h"

// Models
Model* airshipModel = nullptr;
Model* houseModel = nullptr;
Model* treeModel = nullptr;  // for decor
Model* cloudModel = nullptr;
Model* balloonModel = nullptr;
Model* presentModel = nullptr;
Model* groundModel = nullptr;

// Instances
ModelInstance* airshipInstance = nullptr;
std::vector<ModelInstance*> houseInstances;
std::vector<ModelInstance*> treeInstances;
std::vector<ModelInstance*> cloudInstances;
std::vector<ModelInstance*> balloonInstances;
std::vector<ModelInstance*> presentInstances;

Shader* shader = nullptr;
Camera* camera = nullptr;

// Directional light parameters (sun)
glm::vec3 dirLightDirection = glm::vec3(-0.5f, -1.0f, -0.3f);
glm::vec3 dirLightAmbient = glm::vec3(0.3f, 0.3f, 0.3f);
glm::vec3 dirLightDiffuse = glm::vec3(0.8f, 0.8f, 0.8f);
glm::vec3 dirLightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

// Airship control
glm::vec3 airshipVelocity = glm::vec3(0.0f);
float airshipMaxSpeed = 15.0f;
float airshipAcceleration = 30.0f;
float airshipDrag = 5.0f;

// Camera modes
enum CameraMode { FOLLOW_BEHIND, AIMING_DOWN };
CameraMode cameraMode = FOLLOW_BEHIND;

// Camera offset for follow mode
glm::vec3 followCameraOffset = glm::vec3(0.0f, 3.0f, -10.0f);
glm::vec3 aimingCameraOffset =
    glm::vec3(0.0f, -2.0f, 0.0f);  // Under the airship

// Time for animations
float currentTime = 0.0f;

// Random number generator
std::mt19937 rng;
std::uniform_real_distribution<float> distPos(-50.0f, 50.0f);
std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
std::uniform_real_distribution<float> distScale(0.5f, 1.5f);
std::uniform_real_distribution<float> distCloudHeight(15.0f, 30.0f);
std::uniform_real_distribution<float> distBalloonHeight(10.0f, 25.0f);

// Present dropping
struct Present {
  ModelInstance* instance;
  glm::vec3 velocity;
  bool active;
  float spawnTime;
};

std::vector<Present> presents;
float presentGravity = -9.8f;
float presentDespawnHeight = -1.0f;  // Below ground level
float presentDespawnTime = 5.0f;     // Seconds after hitting ground

// Animation parameters
float windStrength = 0.5f;
float windFrequency = 0.5f;

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
  glClearColor(0.53f, 0.81f, 0.98f, 1.0f);  // Sky blue
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  std::cout << "OpenGL initialized" << std::endl;
  std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
}

void initResources() {
  // Initialize random generator
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  rng.seed(seed);

  // Load models (using available models or creating fallback)
  airshipModel = new Model("models/airship.obj");
  if (airshipModel->vertices.empty()) {
    // If airship model doesn't exist, use chair as placeholder
    delete airshipModel;
    airshipModel = new Model("models/chair.obj");
  }
  airshipModel->loadTexture("textures/chair.png");

  houseModel = new Model("models/house.obj");
  if (houseModel->vertices.empty()) {
    // Use table as house placeholder
    delete houseModel;
    houseModel = new Model("models/table.obj");
  }
  houseModel->loadTexture("textures/table.png");

  treeModel = new Model("models/tree.obj");
  if (treeModel->vertices.empty()) {
    // Use vase as tree placeholder
    delete treeModel;
    treeModel = new Model("models/vase.obj");
  }
  treeModel->loadTexture("textures/vase.png");

  cloudModel = new Model("models/cloud.obj");
  if (cloudModel->vertices.empty()) {
    // Create simple cloud model (sphere)
    delete cloudModel;
    cloudModel = new Model("models/sphere.obj");
    if (cloudModel->vertices.empty()) {
      // Final fallback: use cube
      delete cloudModel;
      cloudModel = new Model("models/cube.obj");
    }
  }
  cloudModel->loadTexture("textures/sphere.jpg");

  balloonModel = new Model("models/balloon.obj");
  if (balloonModel->vertices.empty()) {
    // Use sphere for balloon
    delete balloonModel;
    balloonModel = new Model("models/sphere.obj");
    if (balloonModel->vertices.empty()) {
      delete balloonModel;
      balloonModel = new Model("models/cube.obj");
    }
  }
  balloonModel->loadTexture("textures/sphere.jpg");

  presentModel = new Model("models/cube.obj");
  presentModel->loadTexture("textures/cube.jpg");

  // Create ground (simple plane)
  groundModel = new Model("models/cube.obj");
  groundModel->loadTexture("textures/table.png");

  // Create airship instance
  airshipInstance = airshipModel->createInstance();
  airshipInstance->setPosition(glm::vec3(0.0f, 20.0f, 0.0f));
  airshipInstance->setScale(glm::vec3(0.5f, 0.5f, 0.5f));

  // Create houses (5 instances)
  for (int i = 0; i < 5; i++) {
    ModelInstance* house = houseModel->createInstance();
    float x = distPos(rng);
    float z = distPos(rng);
    house->setPosition(glm::vec3(x, 0.0f, z));
    house->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), distRot(rng));
    float scale = distScale(rng) * 0.3f;
    house->setScale(glm::vec3(scale, scale * 1.5f, scale));
    houseInstances.push_back(house);
  }

  // Create decor objects (trees, 5 instances)
  for (int i = 0; i < 5; i++) {
    ModelInstance* tree = treeModel->createInstance();
    float x = distPos(rng);
    float z = distPos(rng);
    tree->setPosition(glm::vec3(x, 0.0f, z));
    tree->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), distRot(rng));
    float scale = distScale(rng) * 0.2f;
    tree->setScale(glm::vec3(scale, scale * 2.0f, scale));
    treeInstances.push_back(tree);
  }

  // Create clouds (5 instances)
  for (int i = 0; i < 5; i++) {
    ModelInstance* cloud = cloudModel->createInstance();
    float x = distPos(rng);
    float z = distPos(rng);
    float y = distCloudHeight(rng);
    cloud->setPosition(glm::vec3(x, y, z));
    cloud->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), distRot(rng));
    float scale = distScale(rng) * 2.0f;
    cloud->setScale(glm::vec3(scale, scale * 0.5f, scale));
    cloudInstances.push_back(cloud);
  }

  // Create balloons (5 instances)
  for (int i = 0; i < 5; i++) {
    ModelInstance* balloon = balloonModel->createInstance();
    float x = distPos(rng);
    float z = distPos(rng);
    float y = distBalloonHeight(rng);
    balloon->setPosition(glm::vec3(x, y, z));
    balloon->setRotation(glm::vec3(0.0f, 1.0f, 0.0f), distRot(rng));
    float scale = distScale(rng) * 0.3f;
    balloon->setScale(glm::vec3(scale, scale * 1.2f, scale));
    balloonInstances.push_back(balloon);
  }

  // Create ground instance
  ModelInstance* ground = groundModel->createInstance();
  ground->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
  ground->setScale(glm::vec3(200.0f, 0.1f, 200.0f));

  shader = new Shader();
  std::cout << "Shader initialized" << std::endl;
}

void updateCamera() {
  if (!airshipInstance) return;

  glm::vec3 airshipPos = airshipInstance->getPosition();
  glm::vec3 cameraOffset;
  glm::vec3 cameraTargetOffset;

  if (cameraMode == FOLLOW_BEHIND) {
    cameraOffset = followCameraOffset;
    cameraTargetOffset =
        glm::vec3(0.0f, -2.0f, 5.0f);  // Look slightly down and ahead
  } else {                             // AIMING_DOWN
    cameraOffset = aimingCameraOffset;
    cameraTargetOffset = glm::vec3(0.0f, -10.0f, 0.0f);  // Look straight down
  }

  // Update camera position and look-at point
  camera->position = airshipPos + cameraOffset;
  glm::vec3 target = airshipPos + cameraTargetOffset;

  // Update camera orientation to look at target
  camera->front = glm::normalize(target - camera->position);
  camera->right =
      glm::normalize(glm::cross(camera->front, glm::vec3(0.0f, 1.0f, 0.0f)));
  camera->up = glm::normalize(glm::cross(camera->right, camera->front));
}

void updatePresents(float deltaTime) {
  for (auto it = presents.begin(); it != presents.end();) {
    if (!it->active) {
      delete it->instance;
      it = presents.erase(it);
      continue;
    }

    // Update velocity with gravity
    it->velocity.y += presentGravity * deltaTime;

    // Update position
    glm::vec3 currentPos = it->instance->getPosition();
    currentPos += it->velocity * deltaTime;
    it->instance->setPosition(currentPos);

    // Check if hit the ground
    if (currentPos.y <= presentDespawnHeight) {
      it->velocity = glm::vec3(0.0f);
      it->spawnTime = currentTime;
      // Mark for removal after delay
      if (currentTime - it->spawnTime > presentDespawnTime) {
        it->active = false;
      }
    }

    ++it;
  }
}

void dropPresent() {
  if (!airshipInstance || !presentModel) return;

  glm::vec3 airshipPos = airshipInstance->getPosition();

  Present newPresent;
  newPresent.instance = presentModel->createInstance();
  newPresent.instance->setPosition(airshipPos + glm::vec3(0.0f, -1.0f, 0.0f));
  newPresent.instance->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
  newPresent.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
  newPresent.active = true;
  newPresent.spawnTime = currentTime;

  presents.push_back(newPresent);
  std::cout << "Present dropped!" << std::endl;
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

  // Set time for vertex shader animations
  shader->setFloat("time", currentTime);
  shader->setFloat("windStrength", windStrength);
  shader->setFloat("windFrequency", windFrequency);

  // Draw airship
  if (airshipModel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, airshipModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 0);  // No animation for airship
    airshipModel->drawAllInstances();
  }

  // Draw houses (no animation)
  if (houseModel) {
    glBindTexture(GL_TEXTURE_2D, houseModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 0);
    houseModel->drawAllInstances();
  }

  // Draw trees (with animation)
  if (treeModel) {
    glBindTexture(GL_TEXTURE_2D, treeModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 1);  // Animate trees
    treeModel->drawAllInstances();
  }

  // Draw clouds (semi-transparent)
  if (cloudModel) {
    glBindTexture(GL_TEXTURE_2D, cloudModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 1);     // Animate clouds
    shader->setFloat("alpha", 0.6f);  // Semi-transparent
    cloudModel->drawAllInstances();
    shader->setFloat("alpha", 1.0f);  // Reset alpha
  }

  // Draw balloons (with gentle animation)
  if (balloonModel) {
    glBindTexture(GL_TEXTURE_2D, balloonModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 1);  // Animate balloons
    balloonModel->drawAllInstances();
  }

  // Draw presents
  if (presentModel) {
    glBindTexture(GL_TEXTURE_2D, presentModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 0);  // No animation for presents
    presentModel->drawAllInstances();
  }

  // Draw ground
  if (groundModel) {
    glBindTexture(GL_TEXTURE_2D, groundModel->texture);
    shader->setInt("textureSampler", 0);
    shader->setInt("animate", 0);
    groundModel->drawAllInstances();
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void handleInput(float deltaTime) {
  if (!airshipInstance) return;

  glm::vec3 airshipPos = airshipInstance->getPosition();
  glm::vec3 movement(0.0f);

  // Horizontal movement (WASD)
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    movement.z -= 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    movement.z += 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    movement.x -= 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    movement.x += 1.0f;
  }

  // Vertical movement (Q/E)
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
    movement.y -= 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
    movement.y += 1.0f;
  }

  // Normalize movement if diagonal
  if (glm::length(movement) > 0.0f) {
    movement = glm::normalize(movement);
  }

  // Update airship velocity with acceleration
  airshipVelocity += movement * airshipAcceleration * deltaTime;

  // Apply drag
  airshipVelocity -= airshipVelocity * airshipDrag * deltaTime;

  // Limit speed
  if (glm::length(airshipVelocity) > airshipMaxSpeed) {
    airshipVelocity = glm::normalize(airshipVelocity) * airshipMaxSpeed;
  }

  // Update position
  airshipPos += airshipVelocity * deltaTime;

  // Keep airship within bounds
  airshipPos.x = glm::clamp(airshipPos.x, -80.0f, 80.0f);
  airshipPos.y = glm::clamp(airshipPos.y, 5.0f, 50.0f);
  airshipPos.z = glm::clamp(airshipPos.z, -80.0f, 80.0f);

  airshipInstance->setPosition(airshipPos);

  // Camera offset adjustment with arrows
  static bool arrowAdjustMode = false;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
    arrowAdjustMode = true;
  } else {
    arrowAdjustMode = false;
  }

  if (arrowAdjustMode && cameraMode == FOLLOW_BEHIND) {
    float adjustSpeed = 5.0f * deltaTime;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      followCameraOffset.y += adjustSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      followCameraOffset.y -= adjustSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      followCameraOffset.x -= adjustSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      followCameraOffset.x += adjustSpeed;
    }
    // PageUp/PageDown for Z offset
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)) {
      followCameraOffset.z += adjustSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) {
      followCameraOffset.z -= adjustSpeed;
    }
  }

  // Drop present with Space
  static bool spacePressed = false;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
    if (!spacePressed) {
      dropPresent();
      spacePressed = true;
    }
  } else {
    spacePressed = false;
  }

  // Toggle camera mode with R
  static bool rPressed = false;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
    if (!rPressed) {
      cameraMode = (cameraMode == FOLLOW_BEHIND) ? AIMING_DOWN : FOLLOW_BEHIND;
      std::cout << "Camera mode: "
                << (cameraMode == FOLLOW_BEHIND ? "Follow" : "Aiming")
                << std::endl;
      rPressed = true;
    }
  } else {
    rPressed = false;
  }

  // Reset camera offset with Backspace
  static bool backspacePressed = false;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Backspace)) {
    if (!backspacePressed) {
      followCameraOffset = glm::vec3(0.0f, 3.0f, -10.0f);
      std::cout << "Camera offset reset" << std::endl;
      backspacePressed = true;
    }
  } else {
    backspacePressed = false;
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

  sf::RenderWindow window(sf::VideoMode(1200, 800), "Airship Delivery Game",
                          sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);
  window.setActive(true);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "GLEW initialization error" << std::endl;
    return -1;
  }

  std::cout << "\n=== AIRSHIP DELIVERY GAME ===" << std::endl;
  std::cout << std::endl;

  initGL();
  initResources();

  // Create camera (will be updated based on airship position)
  camera =
      new Camera(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                 glm::vec3(0.0f, 1.0f, 0.0f));

  // Initial camera update
  updateCamera();

  std::cout << std::endl;
  std::cout << "CONTROLS:" << std::endl;
  std::cout << " W/A/S/D      - Move airship horizontally" << std::endl;
  std::cout << " Q/E          - Move airship up/down" << std::endl;
  std::cout << " SPACE        - Drop present" << std::endl;
  std::cout << " R            - Toggle camera (follow/aiming)" << std::endl;
  std::cout << " LShift + Arrows - Adjust camera offset (follow mode)"
            << std::endl;
  std::cout << " Backspace    - Reset camera offset" << std::endl;
  std::cout << " ESC          - Exit" << std::endl;
  std::cout << std::endl;
  std::cout << "Game features:" << std::endl;
  std::cout << "- 5 randomly placed houses on ground" << std::endl;
  std::cout << "- 5 randomly placed trees (with animation)" << std::endl;
  std::cout << "- 5 randomly placed clouds (semi-transparent)" << std::endl;
  std::cout << "- 5 randomly placed balloons" << std::endl;
  std::cout << "- Drop presents with SPACE" << std::endl;

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
    currentTime += deltaTime;

    handleInput(deltaTime);
    updateCamera();
    updatePresents(deltaTime);
    render(window.getSize().x, window.getSize().y);
    window.display();
  }

  // Cleanup
  delete shader;
  delete camera;
  delete airshipModel;
  delete houseModel;
  delete treeModel;
  delete cloudModel;
  delete balloonModel;
  delete presentModel;
  delete groundModel;

  // Clean up presents
  for (auto& present : presents) {
    delete present.instance;
  }
  presents.clear();

  window.close();
  std::cout << "Program finished" << std::endl;

  return 0;
}