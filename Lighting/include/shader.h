#ifndef SHADER_INSTANCED_H
#define SHADER_INSTANCED_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class InstancedShader {
public:
    GLuint programID;
    
    // Конструктор для инстанцированного шейдера
    InstancedShader();
    
    // Деструктор
    ~InstancedShader();
    
    // Использовать программу
    void use() const;
    
    // Установить uniform переменные
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;
    
private:
    void checkCompileErrors(GLuint shader, const std::string& type);
};

extern const char* orbitVertexShader;
extern const char* orbitFragmentShader;
#endif 