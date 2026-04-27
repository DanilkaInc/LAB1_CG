// Лабораторная работа №6
// Настройка освещения: окружающий, диффузный, зеркальный (модель Фонга)

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader.h"
#include "model.h"

#include <iostream>

// ---------- Параметры окна ----------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ---------- Камера (первая персона) ----------
glm::vec3 cameraPos = glm::vec3(2.0f, 2.0f, 4.0f);   // позиция камеры
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float cameraSpeed = 2.5f;
float sensitivity = 0.1f;

// ---------- Прототипы функций ----------
void processInput(GLFWwindow* window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// ---------- Основная функция ----------
int main() {
    // ======== 1. Инициализация GLFW ========
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab6: Phong Lighting with Model", NULL, NULL);
    if (!window) {
        std::cerr << "Ошибка создания окна GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // захват мыши

    // ======== 2. Загрузка GLAD ========
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Ошибка инициализации GLAD" << std::endl;
        return -1;
    }

    // ======== 3. Глобальные настройки OpenGL ========
    glEnable(GL_DEPTH_TEST);      // проверка глубины
    // glEnable(GL_CULL_FACE);    // при желании включить отбраковку задних граней

    // ======== 4. Загрузка шейдеров ========
    Shader shader("vertex_shader.vert", "fragment_shader.frag");

    // ======== 5. Загрузка 3D-модели (укажите свой путь) ========
    Model ourModel("./3D_model_lab3.glb");   // замените на ваш файл


    // ======== 6. Параметры источника света ========
    glm::vec3 lightPos = glm::vec3(3.0f, 4.0f, 2.0f);
    glm::vec3 lightAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 lightDiffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 lightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

    // ======== 7. Параметры материала (золотистый металл) ========
    glm::vec3 matAmbient = glm::vec3(0.24725f, 0.1995f, 0.0745f);
    glm::vec3 matDiffuse = glm::vec3(0.75164f, 0.60648f, 0.22648f);
    glm::vec3 matSpecular = glm::vec3(0.628281f, 0.555802f, 0.366065f);
    float     matShininess = 51.2f;

    // ======== 8. Основной цикл рендеринга ========
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Обработка ввода (WASD)
        processInput(window, deltaTime);

        // Очистка буферов
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы проекции и вида
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);   // модель в центре сцены

        // Активация шейдера и передача uniform-переменных
        shader.use();

        shader.setUniform("projection", projection);
        shader.setUniform("view", view);
        shader.setUniform("model", model);
        shader.setUniform("viewPos", cameraPos);

        // Свет
        shader.setUniform("light.position", lightPos);
        shader.setUniform("light.ambient", lightAmbient);
        shader.setUniform("light.diffuse", lightDiffuse);
        shader.setUniform("light.specular", lightSpecular);

        // Материал
        shader.setUniform("material.ambient", matAmbient);
        shader.setUniform("material.diffuse", matDiffuse);
        shader.setUniform("material.specular", matSpecular);
        shader.setUniform("material.shininess", matShininess);

        // Рендер модели
        ourModel.Draw(shader);

        // Смена буферов и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ======== 9. Завершение работы ========
    glfwTerminate();
    return 0;
}

// ---------- Реализация ввода ----------
void processInput(GLFWwindow* window, float deltaTime) {
    float speed = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;   // переворот для удобства
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
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