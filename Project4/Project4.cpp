// Лабораторная работа №4
// Движение камеры, матрицы, GLM. Объект: эллипс.

#include <iostream>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>

// ---------- Параметры окна ----------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ---------- Камера ----------
glm::vec3 cameraPos = glm::vec3(2.0f, 1.5f, 2.0f);   // начальное положение сбоку
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

// ---------- Класс шейдера ----------
class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexSource, const char* fragmentSource) {
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexSource, nullptr);
        glCompileShader(vs);
        checkCompileErrors(vs, "VERTEX");
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentSource, nullptr);
        glCompileShader(fs);
        checkCompileErrors(fs, "FRAGMENT");
        ID = glCreateProgram();
        glAttachShader(ID, vs);
        glAttachShader(ID, fs);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(vs);
        glDeleteShader(fs);
    }
    void use() { glUseProgram(ID); }
    void setUniform(const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    }
    void setUniform(const std::string& name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }
private:
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "Ошибка компиляции шейдера (" << type << "):\n" << infoLog << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "Ошибка линковки программы:\n" << infoLog << std::endl;
            }
        }
    }
};

// ---------- Структура вершины (позиция + нормаль) ----------
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

// ---------- Генерация эллипсоида с нормалями ----------
void generateEllipsoid(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
    float a, float b, float c, int slices, int stacks) {
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * i / stacks;          // широта 0..pi
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * j / slices; // долгота 0..2pi
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            float x = a * sinPhi * cosTheta;
            float y = b * cosPhi;
            float z = c * sinPhi * sinTheta;

            // Нормаль для эллипсоида: (x/a?, y/b?, z/c?) после нормализации
            glm::vec3 rawNormal(x / (a * a), y / (b * b), z / (c * c));
            glm::vec3 norm = glm::normalize(rawNormal);

            vertices.push_back({ glm::vec3(x, y, z), norm });
        }
    }

    // Индексы для двух треугольников на каждый прямоугольник сетки
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

// ---------- Главная функция ----------
int main() {
    // 1. Инициализация GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Ellipsoid with Lighting", NULL, NULL);
    if (!window) {
        std::cerr << "Не удалось создать окно" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 2. Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Ошибка инициализации GLAD" << std::endl;
        return -1;
    }

    // 3. Шейдеры с освещением (Ламберт)
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        out vec3 FragPos;
        out vec3 Normal;
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 410 core
        in vec3 FragPos;
        in vec3 Normal;
        out vec4 FragColor;
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;
        void main() {
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            vec3 ambient = 0.3 * lightColor;
            vec3 result = (ambient + diffuse) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    Shader shader(vertexShaderSource, fragmentShaderSource);

    // 4. Генерация эллипсоида (полуоси: a=0.5, b=0.3, c=0.4)
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateEllipsoid(vertices, indices, 0.5f, 0.3f, 0.4f, 60, 40);

    // 5. Создание VAO, VBO, EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO: interleaved (pos + normal)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Атрибут позиции (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    // Атрибут нормали (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 6. Настройки OpenGL
    glEnable(GL_DEPTH_TEST);

    // 7. Параметры источника света
    glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor = glm::vec3(0.2f, 0.7f, 0.9f);  // голубовато-зелёный

    // 8. Основной цикл
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f); // эллипсоид в центре

        shader.use();
        shader.setUniform("projection", projection);
        shader.setUniform("view", view);
        shader.setUniform("model", model);
        shader.setUniform("lightPos", lightPos);
        shader.setUniform("lightColor", lightColor);
        shader.setUniform("objectColor", objectColor);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 9. Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
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
    float yoffset = lastY - ypos; // инверсия для естественного движения
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