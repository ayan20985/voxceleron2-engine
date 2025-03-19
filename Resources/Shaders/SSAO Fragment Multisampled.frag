#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (constant_id = 0) const int SSAO_KERNEL_SIZE = 8;
layout (constant_id = 1) const float SSAO_RADIUS = 1.f;

layout(set = 0, binding = 0) uniform sampler2DMS position_sampler;
layout(set = 0, binding = 1) uniform sampler2DMS normal_sampler;
layout(set = 0, binding = 2) uniform sampler2D ssao_noise_sampler;
layout(set = 0, binding = 3) uniform Uniforms { vec4 samples[SSAO_KERNEL_SIZE]; mat4 projection; } uniforms;

layout(location = 0) in vec2 fragment_uv;

layout(location = 0) out float out_color;

vec4 textureMS(in sampler2DMS texture, vec2 uv)
{
	ivec2 texture_size = textureSize(texture);
	ivec2 texel_uv = ivec2(texture_size*clamp(uv, 0, 1));
	return texelFetch(texture, texel_uv, 0);
}

void main()
{
	//Skip sky
	float position_alpha = textureMS(position_sampler, fragment_uv).a;
	if(position_alpha == 0){ out_color = 1; return; }
	
	//Vectors
	vec3 position = textureMS(position_sampler, fragment_uv).xyz;
	vec3 normal = normalize(textureMS(normal_sampler, fragment_uv).xyz*2.f-1.f);

	//Noise
	vec2 resolution = textureSize(position_sampler);
	vec2 noise_resolution = textureSize(ssao_noise_sampler, 0);
	vec2 noise_uv = fragment_uv*(resolution/noise_resolution);
	vec3 random_vector = normalize(texture(ssao_noise_sampler, noise_uv).xyz*2.f-1.f);

	//TBN
	vec3 tangent = normalize(random_vector-normal*dot(random_vector, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 tbn = mat3(tangent, bitangent, normal);

	//SSAO
	float occlusion = 0.f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; ++i)
	{
		vec3 sample_position = tbn*uniforms.samples[i].xyz;
		sample_position = position+sample_position*SSAO_RADIUS;
		
		vec4 offset = vec4(sample_position, 1.f);
		offset = uniforms.projection*offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz*.5f+.5f;

		float sample_depth = textureMS(position_sampler, offset.xy).z;

		float range_check = smoothstep(0.f, 1.f, SSAO_RADIUS/abs(position.z-sample_depth));
		occlusion += (sample_depth >= sample_position.z ? 1.f : 0.f)*range_check;
	}
	
	out_color = 1.f-occlusion/float(SSAO_KERNEL_SIZE);
}
