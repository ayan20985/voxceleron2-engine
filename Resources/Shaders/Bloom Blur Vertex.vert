#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec2 fragment_uv;

out gl_PerVertex{ vec4 gl_Position; };

void main()
{
	fragment_uv = vec2((gl_VertexIndex<<1)&2, gl_VertexIndex&2);
	gl_Position = vec4(fragment_uv*2.f+-1.f, 0.f, 1.f);
}