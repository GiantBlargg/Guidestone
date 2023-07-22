#version 460

layout(location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform _ { mat4 camera; };

void main() { gl_Position = camera * vec4(pos, 1.0); }
