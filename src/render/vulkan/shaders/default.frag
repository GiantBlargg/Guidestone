#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 colour;

void main() {
	vec3 normal = normalize(in_normal);
	colour = vec4((vec3(1) + normal) * 0.5, 1);
}
