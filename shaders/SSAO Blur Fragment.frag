#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform sampler2D ssao_sampler;

layout(location = 0) in vec2 fragment_uv;

layout(location = 0) out float out_color;

void main()
{
	int range = 2;
	int iterations = 0;
	vec2 texel_size = 1.f/vec2(textureSize(ssao_sampler, 0));
	float result = 0.f;
	for(int x = -2; x < 2; ++x)
	{
		for(int y = -2; y < 2; ++y)
		{
			vec2 offset = vec2(float(x), float(y))*texel_size;
			result += texture(ssao_sampler, fragment_uv+offset).r;
			++iterations;
		}
	}
	out_color = result/float(iterations);
}