#ifndef MODEL_H
#define MODEL_H

#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

struct ModelVertex {
  glm::vec3 position;
  glm::vec2 texCoord;
  glm::vec3 normal;

  bool operator==(const ModelVertex& other) const {
    return position == other.position && texCoord == other.texCoord &&
           normal == other.normal;
  }
};

class ModelInstance;

class Model {
 public:
  // Mesh
  std::vector<ModelVertex> vertices;
  std::vector<GLuint> indices;
  GLuint texture = 0;

  // Конструктор из файла с obj-моделью
  Model(const std::string& filename);

  // Загрузить текстуру
  void loadTexture(const std::string& filename);

  // Создать новый экземпляр
  ModelInstance* createInstance();

  // Получить все экземпляры
  const std::vector<ModelInstance*>& getInstances() const { return instances; }

  // Нарисовать все экземпляры
  void drawAllInstances() const;

  // Деструктор
  ~Model();

 private:
  // Буферы
  GLuint VAO = 0, VBO = 0, EBO = 0;
  mutable GLuint instanceVBO = 0;
  size_t indexCount = 0;

  // Экземпляры модели
  std::vector<ModelInstance*> instances;

  // Буферы для матриц преобразований
  mutable std::vector<glm::mat4> instanceMatrices;
  mutable bool instanceBufferDirty = true;

  // Приватные методы
  bool checkFile(const std::string& filename);
  bool load(const std::string& filename);
  void setupBuffers();
  void setupInstanceBuffer();
  unsigned int addVertex(const ModelVertex& v);
  bool createFallbackModel();
  void updateInstanceBuffer() const;

  friend class ModelInstance;
};

// Экзмепляр модели
class ModelInstance {
 public:
  ModelInstance(Model* parentModel);
  ~ModelInstance();

  // Преобразования
  void setPosition(const glm::vec3& position);
  void setRotation(const glm::vec3& axis, float angleDegrees);
  void setScale(const glm::vec3& scale);

  // Текущее преобразование модели
  const glm::mat4& getTransform() const { return transform; }
  const glm::vec3& getPosition() const { return position; }
  const glm::vec3& getScale() const { return scale; }
  float getRotationAngle() const { return rotationAngle; }
  const glm::vec3& getRotationAxis() const { return rotationAxis; }

  // Операции над экзмепляром
  void translate(const glm::vec3& translation);
  void rotate(const glm::vec3& axis, float angleDegrees);
  void scaleBy(const glm::vec3& scaling);

  // Обновить матрицу преобразований
  void updateTransform();

 private:
  Model* parentModel;
  glm::mat4 transform;

  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);
  glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
  float rotationAngle = 0.0f;

  void markInstanceBufferDirty();
};

#endif  // MODEL_H