#version 410 core
layout (location = 0) in vec2 aPos;

uniform vec2 uOffset;   // смещение для движения

void main()
{
    vec2 pos = aPos + uOffset;
    gl_Position = vec4(pos, 0.0, 1.0);
}