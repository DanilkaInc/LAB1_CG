#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>

#include "shader.h"
#include "model.h"

// --- Параметры окна ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// --- Камера ---
glm::vec3 cameraPos = glm::vec3(3.0f, 2.0f, 4.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float cameraSpeed = 2.5f;
float sensitivity = 0.1f;

// --- НОВЫЕ ПАРАМЕТРЫ ПРЕОБРАЗОВАНИЙ ---
float tablePosX = 0.0f;   // рабочий стол по X (влево-вправо)
float tablePosY = 0.0f;   // рабочий стол по Y (вверх-вниз)
float spindlePosZ = 0.0f; // шпиндель по Z (вперёд-назад)

// --- Структуры для освещения (без изменений) ---
struct Light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

// --- Прототипы функций ---
void processInput(GLFWwindow* window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    // ========== 1. GLFW ==========
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab7: CNC Machine (Table XY, Spindle Z)", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ========== 2. GLAD ==========
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ========== 3. OpenGL настройки ==========
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE); // опционально

    // ========== 4. Шейдер ==========
    Shader shader("vertex_shader.vert", "fragment_shader.frag");

    // ========== 5. Загрузка модели (указать правильный путь) ==========
    Model machine("./3D_model_lab7.glb");
    std::cout << "Loaded " << machine.getMeshCount() << " meshes." << std::endl;
    std::cout << "Mesh names:" << std::endl;
    for (const auto& name : machine.getMeshNames()) {
        std::cout << "  - " << name << std::endl;
    }

    // ========== 6. Корректирующая матрица (при необходимости) ==========
    // Поворот на -90° вокруг X, если модель лежит на боку
    glm::mat4 fix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // ========== 7. Освещение и материал ==========
    Light light;
    light.position = glm::vec3(5.0f, 5.0f, 3.0f);
    light.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    light.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    light.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    Material material;
    material.ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    material.diffuse = glm::vec3(0.6f, 0.6f, 0.6f);
    material.specular = glm::vec3(0.3f, 0.3f, 0.3f);
    material.shininess = 32.0f;

    // ========== 8. Основной цикл ==========
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы проекции и вида
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        shader.use();
        shader.setUniform("projection", projection);
        shader.setUniform("view", view);
        shader.setUniform("viewPos", cameraPos);
        shader.setUniform("light.position", light.position);
        shader.setUniform("light.ambient", light.ambient);
        shader.setUniform("light.diffuse", light.diffuse);
        shader.setUniform("light.specular", light.specular);
        shader.setUniform("material.ambient", material.ambient);
        shader.setUniform("material.diffuse", material.diffuse);
        shader.setUniform("material.specular", material.specular);
        shader.setUniform("material.shininess", material.shininess);

        // --- Применение аффинных преобразований ---
        // Станина (Base) – только коррекция
        machine.setMeshTransform("Base", fix);

        // Рабочий стол: перемещение по X и Y (вверх-вниз = ось Y в OpenGL)
        glm::mat4 tableTransform = fix * glm::translate(glm::mat4(1.0f), glm::vec3(tablePosX, tablePosY, 0.0f));
        machine.setMeshTransform("Table", tableTransform);

        // Шпиндель: перемещение по Z
        glm::mat4 spindleTransform = fix * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, spindlePosZ));
        machine.setMeshTransform("Spindle", spindleTransform);

        machine.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// --- Обработка ввода ---
void processInput(GLFWwindow* window, float deltaTime) {
    float speed = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    float step = 2.0f * deltaTime; // скорость движения деталей

    // Рабочий стол: стрелки влево/вправо (X), вверх/вниз (Y)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  tablePosX -= step;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) tablePosX += step;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    tablePosY += step;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  tablePosY -= step;

    // Шпиндель: PageUp / PageDown – движение по Z
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)   spindlePosZ += step;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) spindlePosZ -= step;

    // Ограничения (подберите под свою модель)
    if (tablePosX < -2.0f) tablePosX = -2.0f;
    if (tablePosX > 2.0f) tablePosX = 2.0f;
    if (tablePosY < -1.0f) tablePosY = -1.0f;
    if (tablePosY > 1.0f) tablePosY = 1.0f;
    if (spindlePosZ < -2.0f) spindlePosZ = -2.0f;
    if (spindlePosZ > 2.0f) spindlePosZ = 2.0f;
}

// --- Мышь для камеры (без изменений) ---
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}