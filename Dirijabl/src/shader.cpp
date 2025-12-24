#include "shader.h"

const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in mat4 instanceMatrix; // Instanced transform

uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float windStrength;
uniform float windFrequency;
uniform int animate; // 0 = no animation, 1 = animate

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out float Alpha;

void main() {
    vec3 animatedPosition = position;
    
    // Apply wind animation to trees, clouds, and balloons
    if (animate == 1) {
        float windEffect = 0.0;
        
        // Different animations based on object type
        // Trees: swaying motion
        if (abs(position.y) > 0.5) { // Assuming trees are tall
            windEffect = sin(time * windFrequency + position.x * 0.1) * 
                        windStrength * 0.1 * (position.y * 0.5);
            animatedPosition.x += windEffect;
            animatedPosition.z += cos(time * windFrequency * 0.8 + position.z * 0.1) * 
                                windStrength * 0.05 * (position.y * 0.5);
        }
        // Clouds: gentle floating
        else if (abs(position.y) < 0.5 && abs(position.x) < 1.0) { // Assuming clouds are flat
            windEffect = sin(time * windFrequency * 0.3 + position.x) * 
                        windStrength * 0.05;
            animatedPosition.x += windEffect;
            animatedPosition.y += cos(time * windFrequency * 0.4 + position.z) * 
                                windStrength * 0.02;
        }
        // Balloons: gentle bobbing
        else {
            windEffect = sin(time * windFrequency * 0.5 + position.x) * 
                        windStrength * 0.03;
            animatedPosition.y += windEffect;
        }
    }
    
    // Apply instance transformation
    vec4 worldPosition = instanceMatrix * vec4(animatedPosition, 1.0);
    
    gl_Position = projection * view * worldPosition;

    TexCoord = texCoord;
    Normal = mat3(transpose(inverse(instanceMatrix))) * normal;
    FragPos = vec3(worldPosition);
    Alpha = 1.0;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in float Alpha;

out vec4 FragColor;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D textureSampler;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform float alpha = 1.0; // Override alpha for specific objects

void main() {
    vec4 texColor = texture(textureSampler, TexCoord);
    
    // Use the smaller of the two alpha values
    float finalAlpha = min(texColor.a, alpha * Alpha);
    if (finalAlpha < 0.1) discard;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-dirLight.direction);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // Ambient
    vec3 ambient = dirLight.ambient * texColor.rgb;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = dirLight.diffuse * diff * texColor.rgb;
    
    // Specular (reduced for more natural look)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = dirLight.specular * spec * vec3(0.3);
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, finalAlpha);
}
)";

Shader::Shader() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
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

Shader::~Shader() {
    glDeleteProgram(programID);
}

void Shader::use() const {
    glUseProgram(programID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()),
        1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &vec[0]);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader compilation error " << type << ":" << std::endl
                << infoLog << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Program linking error:" << std::endl
                << infoLog << std::endl;
        }
    }
}