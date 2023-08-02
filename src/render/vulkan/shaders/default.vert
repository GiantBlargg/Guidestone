#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform _ { mat4 camera; };

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

void main() {
	out_pos = in_pos;
	gl_Position = camera * vec4(out_pos, 1.0);
	out_normal = in_normal;
	out_uv = in_uv;
}
