#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ---------- Класс Shader для загрузки и управления шейдерами (задание №2) ----------
class Shader {
public:
    unsigned int ID;

    // Конструктор: загружает и компилирует шейдеры из файлов
    Shader(const char* vertexPath, const char* fragmentPath) {
        // Чтение файлов
        std::string vertexCode, fragmentCode;
        std::ifstream vShaderFile, fShaderFile;
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vStream, fStream;
            vStream << vShaderFile.rdbuf();
            fStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vertexCode = vStream.str();
            fragmentCode = fStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // Компиляция вершинного шейдера
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Компиляция фрагментного шейдера
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Создание программы
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Очистка
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const { glUseProgram(ID); }

    // Утилиты для uniform (одна строка – задание №2)
    void setUniform(const std::string& name, float v0) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), v0);
    }
    void setUniform(const std::string& name, float v0, float v1) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), v0, v1);
    }
    void setUniform(const std::string& name, float v0, float v1, float v2) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), v0, v1, v2);
    }
    void setUniform(const std::string& name, float v0, float v1, float v2, float v3) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), v0, v1, v2, v3);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
            }
        }
    }
};

// ---------- Генерация вершин и индексов для эллипса ----------
void generateEllipseData(std::vector<float>& vertices, std::vector<unsigned int>& indices, int segments, float rx, float ry) {
    // Вершина 0 – центр эллипса
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    // Вершины на границе
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.1415926f * i / segments;
        float x = rx * cos(angle);
        float y = ry * sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
    }
    // Индексы для треугольников (центр + две соседние граничные вершины)
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);          // центр
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    // Замыкающий треугольник
    indices.push_back(0);
    indices.push_back(segments);
    indices.push_back(1);
}

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "ERROR: could not start GLFW3\n";
        return 1;
    }

    // Настройка OpenGL версии 4.6 Core (по заданию)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Lab2 - Ellipse with VBO/VAO/EBO", NULL, NULL);
    if (!window) {
        std::cerr << "ERROR: could not create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "ERROR: GLEW initialization failed\n";
        return 1;
    }

    // ---------- Создание VAO, VBO, EBO ----------
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Генерация данных эллипса (радиусы 0.5 и 0.3, 100 сегментов)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateEllipseData(vertices, indices, 100, 0.5f, 0.3f);

    glBindVertexArray(VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Атрибут позиции (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ---------- Загрузка шейдеров из файлов (задание №2) ----------
    Shader shader("vertex.glsl", "fragment.glsl");
    shader.use();

    // Переменные для движения и цвета
    float offsetX = 0.0f;
    float speed = 0.5f;   // скорость движения
    float timeAcc = 0.0f;

    // Главный цикл
    while (!glfwWindowShouldClose(window)) {
        // Обновление времени
        float currentTime = glfwGetTime();
        // Движение объекта: смещение по синусоиде (элемент движения)
        offsetX = 0.6f * sin(currentTime * speed);
        shader.setUniform("uOffset", offsetX, 0.0f);

        // Задание №1: изменение цвета со временем (циклично)
        float r = (sin(currentTime) + 1.0f) / 2.0f;      // от 0 до 1
        float g = (sin(currentTime + 2.0f) + 1.0f) / 2.0f;
        float b = (sin(currentTime + 4.0f) + 1.0f) / 2.0f;
        shader.setUniform("uColor", r, g, b, 1.0f);

        // Отрисовка
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}
