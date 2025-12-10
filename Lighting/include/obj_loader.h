#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

struct OBJVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    
    bool operator==(const OBJVertex& other) const {
        return position == other.position && 
               texCoord == other.texCoord && 
               normal == other.normal;
    }
};

class OBJModel {
public:
    std::vector<OBJVertex> vertices;
    std::vector<GLuint> indices;
    
    GLuint VAO = 0, VBO = 0, EBO = 0;
    size_t indexCount = 0;
    
    bool load(const std::string& filename) {
        std::cout << "Загружаем модель из " << filename << std::endl;
        
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Не получилось открыть файл: " << filename << std::endl;
            return createFallbackModel();
        }
        
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
            }
            else if (type == "vt") {
                float u, v;
                iss >> u >> v;
                texCoords.push_back(glm::vec2(u, v));
            }
            else if (type == "vn") {
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(glm::normalize(glm::vec3(x, y, z)));
            }
            else if (type == "f") {
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
                            if (slash2 > slash1 + 1) { // true: 1/1/1 # вершины/текстуры/нормали;       false: 1//1 # вершины//нормали
                                texIdx = std::stoi(vertex.substr(slash1 + 1, slash2 - (slash1 + 1)));
                            }
                            if (slash2 + 1 < vertex.length()) {
                                normIdx = std::stoi(vertex.substr(slash2 + 1));
                            }
                        } 
                        else { // 1/1 # вершины/текстуры
                            if (slash1 + 1 < vertex.length()) {
                                texIdx = std::stoi(vertex.substr(slash1 + 1));
                            }
                        }
                    }
                    
                    OBJVertex v;
                    v.position = positions[posIdx - 1];
                    v.texCoord = texIdx > 0 ? texCoords[texIdx - 1] : glm::vec2(0.0f);
                    v.normal = normIdx > 0 ? normals[normIdx - 1] : glm::vec3(0.0f, 1.0f, 0.0f);
                    
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
    
    void setupBuffers() {
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
        }
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 
                    vertices.size() * sizeof(OBJVertex),
                    vertices.data(),
                    GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    indices.size() * sizeof(GLuint),
                    indices.data(),
                    GL_STATIC_DRAW);
        
        // Layout
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
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    void draw() const {
        if (VAO == 0) return;
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void release() {
        if (EBO != 0) glDeleteBuffers(1, &EBO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        
        vertices.clear();
        indices.clear();
    }
    
    ~OBJModel() {
        release();
    }
    
private:
    std::unordered_map<std::string, unsigned int> vertexMap;
    
    unsigned int addVertex(const OBJVertex& v) {
        // дедупликация
        for (size_t i = 0; i < vertices.size(); i++) {
            if (vertices[i] == v) {
                return i;
            }
        }
        vertices.push_back(v);
        return vertices.size() - 1;
    }
    
    bool createFallbackModel() {
        std::cout << "Создан куб вместо модели" << std::endl;
        
        vertices = {
            // Front
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            // Back
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
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
};