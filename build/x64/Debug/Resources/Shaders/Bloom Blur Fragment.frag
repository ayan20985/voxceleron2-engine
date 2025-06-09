#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform sampler2D bloom_sampler;
layout(set = 0, binding = 1) uniform Uniforms{ uint mode; } uniforms;

layout(location = 0) in vec2 fragment_uv;

layout(location = 0) out vec4 out_color;

const float OFFSETS[3] = float[](.0, 1.3846153846f, 3.2307692308f);
const float WEIGHTS[3] = float[](.2270270270f, .3162162162f, .0702702703f);
const float SPREAD = 3;

void main()
{
	//Separate HDR from LDR
	float threshold = .1f;
	if(uniforms.mode == 0)
	{
		vec3 result = texture(bloom_sampler, fragment_uv).rgb;
		if(result.r > threshold || result.g > threshold ||
			result.b > threshold) out_color = vec4(result, 1);
		else out_color = vec4(0, 0, 0, 0);
		return;
	}

	//Blur
	//Horizontal
	vec3 result = texture(bloom_sampler, fragment_uv).rgb*WEIGHTS[0];
	vec2 texture_offset = 1.f/textureSize(bloom_sampler, 0);
	vec2 scaled_uv = fragment_uv;
	if(uniforms.mode == 1)
		for(int i = 1; i < 3; ++i)
		{
			result += texture(bloom_sampler, scaled_uv+vec2(
				texture_offset.x*OFFSETS[i], 0.f)*SPREAD).rgb*WEIGHTS[i];
			result += texture(bloom_sampler, scaled_uv-vec2(
				texture_offset.x*OFFSETS[i], 0.f)*SPREAD).rgb*WEIGHTS[i];
		}

	//Vertical
	else
		for(int i = 1; i < 3; ++i)
		{
			result += texture(bloom_sampler, scaled_uv+vec2(
				0.f, texture_offset.y*OFFSETS[i])*SPREAD).rgb*WEIGHTS[i];
			result += texture(bloom_sampler, scaled_uv-vec2(
				0.f, texture_offset.y*OFFSETS[i])*SPREAD).rgb*WEIGHTS[i];
		}

	out_color = vec4(result, 1.f);
}