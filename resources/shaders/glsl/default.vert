#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

layout(set = 1, binding = 0) uniform UniformData {
	mat4 mvp;
};

void main() {
	gl_Position = mvp * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
