#include "model.h"

// Создать и загрузить модель из obj-файла.
Model::Model(const std::string& filename) {
  vertices = {};
  indices = {};
  VAO = 0, VBO = 0, EBO = 0, instanceVBO = 0;
  indexCount = 0;
  load(filename);
}

// Проверить файл на доступность.
bool Model::checkFile(const std::string& filename) {
  std::error_code ec;
  auto file_status = std::filesystem::status(filename, ec);

  if (ec) {
    std::cerr << "File check error: " << ec.message() << std::endl;
    return false;
  }

  if (!std::filesystem::exists(file_status)) {
    std::cerr << "File doesn't exist: " << filename << std::endl;
    return false;
  }

  if (std::filesystem::is_directory(file_status)) {
    std::cerr << "Path is a directory, not a file: " << filename << std::endl;
    return false;
  }

  try {
    auto file_size = std::filesystem::file_size(filename);
    if (file_size == 0) {
      std::cerr << "File is empty: " << filename << std::endl;
      return false;
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Couldn't get file size: " << e.what() << std::endl;
  }

  return true;
}

// Загрузить модель из obj-файла.
bool Model::load(const std::string& filename) {
  std::cout << "Загружаем модель из " << filename << std::endl;

  if (!checkFile(filename)) {
    return createFallbackModel();
  }

  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Не получилось открыть файл: " << filename << std::endl;

    // Проверка доступа к файлу
    auto perms = std::filesystem::status(filename).permissions();
    std::string permission_error;

    if ((perms & std::filesystem::perms::owner_read) ==
            std::filesystem::perms::none &&
        (perms & std::filesystem::perms::group_read) ==
            std::filesystem::perms::none &&
        (perms & std::filesystem::perms::others_read) ==
            std::filesystem::perms::none) {
      permission_error = "Нет прав на чтение файла";
    } else if ((perms & std::filesystem::perms::owner_read) ==
               std::filesystem::perms::none) {
      permission_error = "Нет прав пользователя на чтение";
    }

    if (!permission_error.empty()) {
      std::cerr << permission_error << std::endl;
    }
    return createFallbackModel();
  }

  if (!file.good()) {
    std::cerr << "Поток файла в нерабочем состоянии после открытия"
              << std::endl;
    return createFallbackModel();
  }

  // Приступаем к чтению данных.
  std::vector<glm::vec3> positions;
  std::vector<glm::vec2> texCoords;
  std::vector<glm::vec3> normals;

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::istringstream iss(line);
    std::string type;
    iss >> type;

    if (type == "v") {
      float x, y, z;
      iss >> x >> y >> z;
      positions.push_back(glm::vec3(x, y, z));
    } else if (type == "vt") {
      float u, v;
      iss >> u >> v;
      texCoords.push_back(glm::vec2(u, v));
    } else if (type == "vn") {
      float x, y, z;
      iss >> x >> y >> z;
      normals.push_back(glm::normalize(glm::vec3(x, y, z)));
    } else if (type == "f") {
      std::string vertex;
      std::vector<unsigned int> faceIndices;

      while (iss >> vertex) {
        unsigned int posIdx = 0, texIdx = 0, normIdx = 0;

        // Парсим "v", "v/vt", "v/vt/vn" или "v//vn"
        size_t slash1 = vertex.find('/');

        if (slash1 == std::string::npos) {
          // Только позиция
          posIdx = std::stoi(vertex);
        } else {
          posIdx = std::stoi(vertex.substr(0, slash1));
          size_t slash2 = vertex.find('/', slash1 + 1);

          if (slash2 != std::string::npos) {
            if (slash2 >
                slash1 + 1) {  // true: 1/1/1 # вершины/текстуры/нормали;
                               // false: 1//1 # вершины//нормали
              texIdx =
                  std::stoi(vertex.substr(slash1 + 1, slash2 - (slash1 + 1)));
            }
            if (slash2 + 1 < vertex.length()) {
              normIdx = std::stoi(vertex.substr(slash2 + 1));
            }
          } else {  // 1/1 # вершины/текстуры
            if (slash1 + 1 < vertex.length()) {
              texIdx = std::stoi(vertex.substr(slash1 + 1));
            }
          }
        }

        ModelVertex v;
        v.position = positions[posIdx - 1];
        v.texCoord = texIdx > 0 ? texCoords[texIdx - 1] : glm::vec2(0.0f);
        v.normal =
            normIdx > 0 ? normals[normIdx - 1] : glm::vec3(0.0f, 1.0f, 0.0f);

        unsigned int idx = addVertex(v);
        faceIndices.push_back(idx);
      }

      // Триангуляция
      for (size_t i = 1; i < faceIndices.size() - 1; i++) {
        indices.push_back(faceIndices[0]);
        indices.push_back(faceIndices[i]);
        indices.push_back(faceIndices[i + 1]);
      }
    }
  }

  file.close();

  if (vertices.empty()) {
    std::cerr << "Модель пуста!" << std::endl;
    return createFallbackModel();
  }

  indexCount = indices.size();
  setupBuffers();

  std::cout << "Модель загружена: " << vertices.size() << " вершин, "
            << indexCount << " индексов" << std::endl;
  return true;
}

void Model::loadTexture(const std::string& filename) {
  if (texture != 0) {
    std::cerr << "Текстура уже загружена" << std::endl;
    return;
  }

  if (!checkFile(filename)) {
    return;
  }

  sf::Image image;
  if (image.loadFromFile(filename)) {
    image.flipVertically();
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

    std::cout << "Текстура загружена: " << filename << std::endl;
  } else {
    std::cerr << "Не удалось загрузить текстуру: " << filename << std::endl;
  }
}

ModelInstance* Model::createInstance() {
  ModelInstance* instance = new ModelInstance(this);
  instances.push_back(instance);
  instanceBufferDirty = true;
  return instance;
}

void Model::drawAllInstances() const {
  if (VAO == 0 || instances.empty()) return;

  // Обновить буфер экземпляров, если необходимо
  updateInstanceBuffer();

  glBindVertexArray(VAO);
  glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0,
                          instances.size());
  glBindVertexArray(0);
}

void Model::updateInstanceBuffer() const {
  if (!instanceBufferDirty) return;

  // Сбор всех матриц преобразований
  instanceMatrices.clear();
  for (const auto& instance : instances) {
    instanceMatrices.push_back(instance->getTransform());
  }

  if (instanceMatrices.empty()) return;

  if (instanceVBO == 0) {
    glGenBuffers(1, &instanceVBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // 4 вектора vec4 в качестве mat4
    for (int i = 0; i < 4; i++) {
      glEnableVertexAttribArray(3 + i);
      glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (void*)(i * sizeof(glm::vec4)));
      glVertexAttribDivisor(3 + i, 1);  // Задать обход буфера экземпляров
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
               instanceMatrices.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  instanceBufferDirty = false;
}

void Model::setupBuffers() {
  if (VAO == 0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
  }

  glBindVertexArray(VAO);

  // Вершины
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex),
               vertices.data(), GL_STATIC_DRAW);

  // Полигоны через индексы
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
               indices.data(), GL_STATIC_DRAW);

  // Позиции вершин
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        (void*)offsetof(ModelVertex, position));
  glEnableVertexAttribArray(0);

  // Текстурные координаты
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        (void*)offsetof(ModelVertex, texCoord));
  glEnableVertexAttribArray(1);

  // Нормали
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        (void*)offsetof(ModelVertex, normal));
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

Model::~Model() {
  for (auto instance : instances) {
    delete instance;
  }
  instances.clear();

  if (EBO != 0) glDeleteBuffers(1, &EBO);
  if (VBO != 0) glDeleteBuffers(1, &VBO);
  if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
  if (VAO != 0) glDeleteVertexArrays(1, &VAO);
  if (texture != 0) glDeleteTextures(1, &texture);
  vertices.clear();
  indices.clear();
}

unsigned int Model::addVertex(const ModelVertex& v) {
  for (size_t i = 0; i < vertices.size(); i++) {
    if (vertices[i] == v) {
      return i;
    }
  }
  vertices.push_back(v);
  return vertices.size() - 1;
}

bool Model::createFallbackModel() {
  std::cout << "Создан куб вместо модели" << std::endl;

  vertices = {
      // Front
      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      // Back
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
  };

  indices = {
      0, 1, 2, 0, 2, 3,  // Front
      4, 6, 5, 4, 7, 6,  // Back
      0, 4, 5, 0, 5, 1,  // Bottom
      2, 6, 7, 2, 7, 3,  // Top
      0, 3, 7, 0, 7, 4,  // Left
      1, 5, 6, 1, 6, 2,  // Right
  };

  indexCount = indices.size();
  setupBuffers();

  return true;
}

// ModelInstance

ModelInstance::ModelInstance(Model* parentModel) : parentModel(parentModel) {
  updateTransform();
}

ModelInstance::~ModelInstance() {
  // Надо удалить этот экзмепляр из родительской модели
  if (parentModel) {
    auto& instances =
        const_cast<std::vector<ModelInstance*>&>(parentModel->getInstances());
    instances.erase(std::remove(instances.begin(), instances.end(), this),
                    instances.end());
    parentModel->instanceBufferDirty = true;
  }
}

void ModelInstance::setPosition(const glm::vec3& position) {
  this->position = position;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::setRotation(const glm::vec3& axis, float angleDegrees) {
  this->rotationAxis = glm::normalize(axis);
  this->rotationAngle = angleDegrees;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::setScale(const glm::vec3& scale) {
  this->scale = scale;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::translate(const glm::vec3& translation) {
  position += translation;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::rotate(const glm::vec3& axis, float angleDegrees) {
  rotationAxis = glm::normalize(axis);
  rotationAngle += angleDegrees;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::scaleBy(const glm::vec3& scaling) {
  scale *= scaling;
  updateTransform();
  markInstanceBufferDirty();
}

void ModelInstance::updateTransform() {
  transform = glm::mat4(1.0f);
  transform = glm::translate(transform, position);
  transform = glm::rotate(transform, glm::radians(rotationAngle), rotationAxis);
  transform = glm::scale(transform, scale);
}

void ModelInstance::markInstanceBufferDirty() {
  if (parentModel) {
    parentModel->instanceBufferDirty = true;
  }
}