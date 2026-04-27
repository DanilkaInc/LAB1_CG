#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 FragPos;   // позиция фрагмента в мировых координатах
out vec3 Normal;    // нормаль в мировых координатах

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Нормальная матрица: transpose(inverse(model)) для корректного преобразования нормалей
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}