#version 410 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

// ---- Структура материала ----
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;   // блеск (чем выше, тем меньше и ярче блик)
};

// ---- Структура источника света ----
struct Light {
    vec3 position;     // позиция в мировых координатах
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;  // позиция камеры

void main() {
    // Нормализация
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Ambient (окружающий)
    vec3 ambient = light.ambient * material.ambient;
    
    // Diffuse (диффузный)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    
    // Specular (зеркальный)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);
    
    // Итоговый цвет
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}