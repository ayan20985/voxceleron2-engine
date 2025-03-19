#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 1, binding = 0) uniform sampler2DArray texture_map_sampler;

layout(location = 0) in vec3 fragment_position;
layout(location = 1) in vec2 fragment_uv;
layout(location = 2) in vec3 fragment_normal;
layout(location = 3) flat in float fragment_material;

layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 normal_out;
layout(location = 2) out vec4 albedo_out;
layout(location = 3) out float specular_out;

void main()
{
	position_out = vec4(fragment_position, 1);
	normal_out = vec4(fragment_normal*.5f+.5f, 1);
	albedo_out = vec4(texture(texture_map_sampler,
		  vec3(fragment_uv, (fragment_material-1)*2)).rgb, 1);
	specular_out = texture(texture_map_sampler,
		  vec3(fragment_uv, (fragment_material-1)*2+1)).r;
}
