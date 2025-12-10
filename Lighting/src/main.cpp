#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "shader.h"
#include "obj_loader.h"
#include "camera.h"
#include "solar_system.h"

// =====================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ДЛЯ ОРБИТ
// =====================================================

GLuint orbitShaderProgram = 0;
GLuint orbitVAO = 0, orbitVBO = 0;
std::vector<float> orbitVertices;
std::vector<glm::vec3> orbitColors;
std::vector<int> orbitSegmentStarts;
std::vector<int> orbitSegmentCounts;
bool showOrbits = true;

// =====================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ДЛЯ ИНСТАНЦИРОВАНИЯ
// =====================================================

OBJModel planetModel;              
InstancedShader* instancedShader = nullptr;
Camera* camera = nullptr;
SolarSystem* solarSystem = nullptr;

GLuint sunTexture = 0;
GLuint planetTexture = 0;

GLuint instanceVBO = 0;
GLuint instanceVAO = 0;
size_t instanceCount = 0;

// =====================================================
// ФУНКЦИИ ДЛЯ ОРБИТ
// =====================================================

std::vector<float> createOrbitCircle(float radius, int segments = 100) {
    std::vector<float> vertices;
    for (int i = 0; i <= segments; i++) {
        float angle = (i / (float)segments) * 2.0f * 3.14159265f;
        vertices.push_back(radius * std::cos(angle));  
        vertices.push_back(0.0f);                      
        vertices.push_back(radius * std::sin(angle));  
    }
    return vertices;
}

void initOrbitShader() {  
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &orbitVertexShader, NULL);
    glCompileShader(vertex);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции вертексного шейдера орбит:\n" << infoLog << std::endl;
    }
    
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &orbitFragmentShader, NULL);
    glCompileShader(fragment);
    
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции фрагментного шейдера орбит:\n" << infoLog << std::endl;
    }
    
    orbitShaderProgram = glCreateProgram();
    glAttachShader(orbitShaderProgram, vertex);
    glAttachShader(orbitShaderProgram, fragment);
    glLinkProgram(orbitShaderProgram);
    
    glGetProgramiv(orbitShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(orbitShaderProgram, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки шейдера орбит:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    std::cout << "Шейдер для орбит скомпилирован" << std::endl;
}

void initOrbits() {
    if (!solarSystem) return;
    
    const auto& bodies = solarSystem->getBodies();
    
    std::vector<glm::vec3> colors = {
        glm::vec3(1.0f, 0.5f, 0.0f),  
        glm::vec3(0.0f, 0.8f, 1.0f),  
        glm::vec3(0.0f, 1.0f, 0.5f),  
        glm::vec3(1.0f, 0.0f, 0.5f),  
        glm::vec3(0.5f, 0.0f, 1.0f),  
        glm::vec3(1.0f, 1.0f, 0.0f),  
        glm::vec3(1.0f, 0.8f, 0.0f),  
    };
    
    for (size_t i = 1; i < bodies.size(); i++) {  
        const auto& body = bodies[i];
        if (body.orbitRadius > 0.0f) {
            auto circleVertices = createOrbitCircle(body.orbitRadius);
            
            orbitSegmentStarts.push_back(orbitVertices.size() / 3);  
            orbitSegmentCounts.push_back(circleVertices.size() / 3); 
            
            orbitVertices.insert(orbitVertices.end(), 
                                circleVertices.begin(), 
                                circleVertices.end());
            
            int colorIndex = (i - 1) % colors.size();
            orbitColors.push_back(colors[colorIndex]);
        }
    }
    
    if (!orbitVertices.empty()) {
        glGenVertexArrays(1, &orbitVAO);
        glGenBuffers(1, &orbitVBO);
        
        glBindVertexArray(orbitVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
        glBufferData(GL_ARRAY_BUFFER, 
                    orbitVertices.size() * sizeof(float),
                    orbitVertices.data(),
                    GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
                            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void renderOrbits(const glm::mat4& view, const glm::mat4& projection) {
    if (!showOrbits || orbitVAO == 0 || orbitShaderProgram == 0) return;
    
    glUseProgram(orbitShaderProgram);
    
    GLint viewLoc = glGetUniformLocation(orbitShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(orbitShaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(orbitShaderProgram, "color");
    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    glDisable(GL_DEPTH_TEST);
    
    glLineWidth(1.5f);
    
    glBindVertexArray(orbitVAO);
    
    for (size_t i = 0; i < orbitSegmentStarts.size(); i++) {
        glUniform3fv(colorLoc, 1, glm::value_ptr(orbitColors[i]));
        glDrawArrays(GL_LINE_STRIP, 
                    orbitSegmentStarts[i], 
                    orbitSegmentCounts[i]);
    }
    
    glBindVertexArray(0);
    
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);
}

// =====================================================
// ФУНКЦИИ ДЛЯ ИНСТАНЦИРОВАННОГО РЕНДЕРИНГА
// =====================================================

void setupInstancedRendering() {
    glGenVertexArrays(1, &instanceVAO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(instanceVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planetModel.VBO);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(OBJVertex),
                        (void*)offsetof(OBJVertex, position));
    glEnableVertexAttribArray(0);

    // TexCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                        sizeof(OBJVertex),
                        (void*)offsetof(OBJVertex, texCoord));
    glEnableVertexAttribArray(1);

    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                        sizeof(OBJVertex),
                        (void*)offsetof(OBJVertex, normal));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                        sizeof(glm::mat4),
                        (void*)0);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
                        sizeof(glm::mat4),
                        (void*)(sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE,
                        sizeof(glm::mat4),
                        (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE,
                        sizeof(glm::mat4),
                        (void*)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(6);

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planetModel.EBO);

    glBindVertexArray(0);
}

void updateInstanceBuffer() {
    if (solarSystem == nullptr) return;

    std::vector<glm::mat4> modelMatrices = solarSystem->getModelMatrices();
    instanceCount = modelMatrices.size();

    if (instanceCount == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                instanceCount * sizeof(glm::mat4),
                modelMatrices.data(),
                GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// =====================================================
// ИНИЦИАЛИЗАЦИЯ
// =====================================================

void initGL() {
    glClearColor(66.0f / 255.0f, 133.0f / 255.0f, 180.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    std::cout << " OpenGL инициализирован" << std::endl;
    std::cout << "  Версия: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
}

void initShaders() {
    instancedShader = new InstancedShader();
    std::cout << "Инстанцированные шейдеры скомпилированы" << std::endl;    
    initOrbitShader();
}

GLuint loadSimpleTexture(const std::string& filename) {
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                    size.x, size.y,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        std::cout << "Текстура загружена: " << filename << std::endl;
        return texture;
    }

    std::cout << "Использую fallback текстуру для: " << filename << std::endl;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    unsigned char data[4 * 4 * 3];
    for (int i = 0; i < 4 * 4 * 3; i += 3) {
        if (filename.find("sun") != std::string::npos) {
            data[i] = 255;
            data[i+1] = 200; 
            data[i+2] = 0;   
        } else {
            data[i] = 100 + rand() % 156;
            data[i+1] = 100 + rand() % 156;
            data[i+2] = 100 + rand() % 156;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void initModelsAndSystem() {
    if (!planetModel.load("models/fish.obj")) {
        std::cerr << "Ошибка загрузки модели планеты" << std::endl;
    } else {
        std::cout << "Модель планеты загружена: "
                  << planetModel.vertices.size() << " вершин, "
                  << planetModel.indices.size() << " индексов" << std::endl;
    }

    sunTexture = loadSimpleTexture("textures/fish.png");
    planetTexture = loadSimpleTexture("textures/fish.png");

    setupInstancedRendering();

    solarSystem = new SolarSystem();


    CelestialBody sun;
    sun.orbitRadius = 0.0f;
    sun.orbitSpeed = 0.0f;
    sun.rotationSpeed = 0.5f;
    sun.scale = 15.0f;
    sun.orbitCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    solarSystem->addBody(sun);

    float radii[] = {4.0f, 6.0f, 8.0f, 10.0f, 13.0f, 16.0f};
    float speeds[] = {3.0f, 2.0f, 1.5f, 1.2f, 0.8f, 0.5f};
    float rotations[] = {4.0f, 3.0f, 2.5f, 2.0f, 1.5f, 1.0f};
    float scales[] = {5.3f, 5.1f, 5.6f, 6.4f, 4.2f, 4.8f};

    for (int i = 0; i < 6; i++) {
        CelestialBody planet;
        planet.orbitRadius = radii[i];
        planet.orbitSpeed = speeds[i];
        planet.rotationSpeed = rotations[i];
        planet.scale = scales[i];
        planet.orbitCenter = glm::vec3(0.0f, 0.0f, 0.0f);
        solarSystem->addBody(planet);
    }

    std::cout << "Солнечная система инициализирована (" << solarSystem->getBodyCount() << " объектов)" << std::endl;

    updateInstanceBuffer();    
    initOrbits();
}

// =====================================================
// ОСНОВНОЙ ЦИКЛ
// =====================================================

void render(float width, float height) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix(width / height);

    // 1. Рисуем орбиты 
    renderOrbits(view, projection);

    // 2. Рисуем планеты
    if (!instancedShader) return;

    instancedShader->use();

    // Позиция света (Солнце)
    glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);

    instancedShader->setMat4("view", view);
    instancedShader->setMat4("projection", projection);
    instancedShader->setVec3("lightPos", lightPos);

    updateInstanceBuffer();

    // Рисуем все инстансы за один вызов
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planetTexture);
    instancedShader->setInt("textureSampler", 0);

    glBindVertexArray(instanceVAO);
    glDrawElementsInstanced(GL_TRIANGLES,
                           planetModel.indexCount,
                           GL_UNSIGNED_INT,
                           0,
                           instanceCount);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

// =====================================================
// УПРАВЛЕНИЕ КАМЕРОЙ И ОРБИТАМИ
// =====================================================

void handleInput(float deltaTime) {
    float moveSpeed = 5.0f * deltaTime;
    float rotateSpeed = 50.0f * deltaTime;

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

    static bool oKeyPressed = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
        if (!oKeyPressed) {
            showOrbits = !showOrbits;
            std::cout << "Орбиты: " << (showOrbits ? "ВКЛ" : "ВЫКЛ") << std::endl;
            oKeyPressed = true;
        }
    } else {
        oKeyPressed = false;
    }

    static bool rKeyPressed = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        if (!rKeyPressed) {
            delete camera;
            camera = new Camera(glm::vec3(0.0f, 10.0f, 30.0f));
            std::cout << "Камера сброшена в начальную позицию" << std::endl;
            rKeyPressed = true;
        }
    } else {
        rKeyPressed = false;
    }
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window(sf::VideoMode(1200, 800), "Solar System - Инстанцированный рендеринг",
                           sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Ошибка инициализации GLEW" << std::endl;
        return -1;
    }

    std::cout << "=== СОЛНЕЧНАЯ СИСТЕМА ===" << std::endl;
    std::cout << std::endl;

    initGL();
    initShaders();
    initModelsAndSystem();
    camera = new Camera(glm::vec3(0.0f, 10.0f, 30.0f));

    std::cout << std::endl;
    std::cout << "  УПРАВЛЕНИЕ:" << std::endl;
    std::cout << "  W/A/S/D - движение вперёд/назад/влево/вправо" << std::endl;
    std::cout << "  SPACE/CTRL - движение вверх/вниз" << std::endl;
    std::cout << "  СТРЕЛКИ - повороты камеры" << std::endl;
    std::cout << "  O - показать/скрыть орбиты" << std::endl;
    std::cout << "  R - сбросить камеру в начальную позицию" << std::endl;
    std::cout << "  ESC - выход" << std::endl;
    std::cout << std::endl;

    sf::Clock clock;
    bool running = true;
    float frameTime = 0.0f;
    int frameCount = 0;

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
        frameTime += deltaTime;
        frameCount++;

        handleInput(deltaTime);
        solarSystem->update(deltaTime * 10.0f);

        render(window.getSize().x, window.getSize().y);

        window.display();
    }

    delete instancedShader;
    delete camera;
    delete solarSystem;
    glDeleteTextures(1, &sunTexture);
    glDeleteTextures(1, &planetTexture);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &instanceVAO);
    
    glDeleteProgram(orbitShaderProgram);
    glDeleteBuffers(1, &orbitVBO);
    glDeleteVertexArrays(1, &orbitVAO);

    window.close();
    std::cout << "✅ Программа завершена" << std::endl;

    return 0;
}