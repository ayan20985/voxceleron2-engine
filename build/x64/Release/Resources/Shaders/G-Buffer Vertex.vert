#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform Uniform { mat4 model, view, projection;} uniforms;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in float vertex_material;

layout(location = 0) out vec3 fragment_position;
layout(location = 1) out vec2 fragment_uv;
layout(location = 2) out vec3 fragment_normal;
layout(location = 3) out float fragment_material;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
	gl_Position = uniforms.projection*uniforms.view*uniforms.model*vec4(vertex_position, 1);

	fragment_uv = vertex_uv;

	fragment_position = vec3(uniforms.view*uniforms.model*vec4(vertex_position, 1));

	const mat3 NORMAL_MATRIX = transpose(inverse(mat3(uniforms.view*uniforms.model)));
	fragment_normal = NORMAL_MATRIX*vertex_normal;

	fragment_material = vertex_material;
}