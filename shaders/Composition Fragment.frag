#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform sampler2D lighting_sampler;
layout(set = 0, binding = 1) uniform sampler2D bloom_sampler;

layout(location = 0) in vec2 fragment_uv;

layout(location = 0) out vec4 out_color;

vec3 linear_to_gamma_space(vec3 color){ return pow(color, vec3(1/2.2f)); }

void main()
{
	vec3 result = texture(lighting_sampler, fragment_uv).rgb;
	result += texture(bloom_sampler, fragment_uv).rgb/13.f;

	//HDR tonemap
	float exposure = 1.f;
	result = vec3(1.f)-exp(-result*exposure);
	//result /= (result+vec3(1));

	result = linear_to_gamma_space(result);

	out_color = vec4(result, 1);
}