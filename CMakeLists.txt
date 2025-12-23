cmake_minimum_required(VERSION 3.10)
project(Dirijabl-Aga)

# ============================================================================
# C++ Standard
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ============================================================================
# OpenGL Configuration
# ============================================================================
set(OpenGL_GL_PREFERENCE GLVND)

# ============================================================================
# Find Required Packages
# ============================================================================
find_package(OpenGL 3.3 REQUIRED)
find_package(GLEW 2.0 REQUIRED)
find_package(glm REQUIRED)
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# ============================================================================
# Project Structure - Освещение
# ============================================================================
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/src/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/src/shader.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/include/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/include/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dirijabl/include/model.h
)

# ============================================================================
# Проверка существования файлов
# ============================================================================
foreach(src ${SOURCES})
    if(NOT EXISTS ${src})
        message(WARNING "Файл не найден: ${src}")
    else()
        message(STATUS "Найден: ${src}")
    endif()
endforeach()

# ============================================================================
# Create Executable
# ============================================================================
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ============================================================================
# Include Directories
# ============================================================================
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/Lighting/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Lighting/src
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${SFML_INCLUDE_DIR}
)

# ============================================================================
# Link Libraries
# ============================================================================
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        OpenGL::OpenGL           # OpenGL 3.3
        GLEW::GLEW               # GLEW для загрузки функций
        glm::glm                 # GLM для матричных операций
        sfml-graphics            # SFML Graphics (для Image)
        sfml-window              # SFML Window (OpenGL контекст)
        sfml-system              # SFML System
)

# ============================================================================
# Compiler Flags
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# ============================================================================
# Build Output - Исполняемый файл в папке build/bin
# ============================================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# ============================================================================
# Version Information
# ============================================================================
message(STATUS "")
message(STATUS "========== Configuration ==========")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenGL Version: ${OPENGL_VERSION_STRING}")
message(STATUS "GLEW Version: ${GLEW_VERSION}")
message(STATUS "SFML Version: ${SFML_VERSION}")
message(STATUS "GLM Version: ${GLM_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_CURRENT_BINARY_DIR}/bin")
message(STATUS "Models Directory: ${CMAKE_CURRENT_SOURCE_DIR}/Lighting/models")
message(STATUS "Textures Directory: ${CMAKE_CURRENT_SOURCE_DIR}/Lighting/textures")
message(STATUS "================================================")
message(STATUS "")