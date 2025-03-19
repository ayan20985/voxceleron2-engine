#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform Uniform{ mat4 model, view, projection; } uniforms;
layout(set = 1, binding = 0) uniform Shadow_Uniform{ mat4 matrix; } shadow_uniforms;

layout(location = 0) in vec3 vertex_position;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{ gl_Position = shadow_uniforms.matrix*uniforms.model*vec4(vertex_position, 1); }