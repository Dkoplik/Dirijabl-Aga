#include "shader.h"
#include <iostream>

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in mat4 instanceMatrix; // Матрица инстанса (занимает 4 атрибута)

uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

void main() {
    vec4 worldPos = instanceMatrix * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
    
    TexCoord = texCoord;
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(instanceMatrix))) * normal;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D textureSampler;
uniform vec3 lightPos;

out vec4 FragColor;

void main() {
    // Получаем цвет текстуры
    vec4 texColor = texture(textureSampler, TexCoord);
    if (texColor.a < 0.1) discard;
    
    // Фонговое освещение
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Амбиентное освещение
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * texColor.rgb;
    
    // Диффузное освещение
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texColor.rgb;
    
    // Спекулярное освещение
    float specularStrength = 0.5;
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, texColor.a);
}
)";

const char* orbitVertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * vec4(position, 1.0);
        }
    )";
    
const char* orbitFragmentShader = R"(
        #version 330 core
        uniform vec3 color;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";

InstancedShader::InstancedShader() {
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    
    programID = glCreateProgram();
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);
    checkCompileErrors(programID, "PROGRAM");
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

InstancedShader::~InstancedShader() {
    glDeleteProgram(programID);
}

void InstancedShader::use() const {
    glUseProgram(programID);
}

void InstancedShader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 
                      1, GL_FALSE, &mat[0][0]);
}

void InstancedShader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &vec[0]);
}

void InstancedShader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void InstancedShader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void InstancedShader::checkCompileErrors(GLuint shader, const std::string& type) {
    int success;
    char infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Ошибка компиляции " << type << " шейдера:" << std::endl
                     << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Ошибка линкования программы:" << std::endl
                     << infoLog << std::endl;
        }
    }
}