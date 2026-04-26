#version 410 core
out vec4 FragColor;

uniform vec4 uColor;    // цвет, изменяемый со временем

void main()
{
    FragColor = uColor;
}