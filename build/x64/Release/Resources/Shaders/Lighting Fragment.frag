#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform sampler2D position_sampler;
layout(set = 0, binding = 1) uniform sampler2D normal_sampler;
layout(set = 0, binding = 2) uniform sampler2D albedo_sampler;
layout(set = 0, binding = 3) uniform sampler2D specular_sampler;
layout(set = 0, binding = 4) uniform sampler2D translucent_sampler;
layout(set = 0, binding = 5) uniform sampler2D ssao_sampler;
layout(set = 0, binding = 6) uniform sampler2D shadow_sampler;
layout(set = 0, binding = 7) uniform
Uniforms
{
	vec4 camera_position;
	mat4 inverse_view;
	mat4 transposed_view;
	mat4 shadow_matrix;
} uniforms;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_lighting;

const float PI = 3.14159265359f;
const float SHADOW_BIAS = .0005f;

float ggx_distribution(float normal_dot_halfway, float roughness_squared)
{ 
	const float NORMAL_DOT_HALFWAY_SQUARED = normal_dot_halfway*normal_dot_halfway;
	const float DENOMINATOR = NORMAL_DOT_HALFWAY_SQUARED*(roughness_squared-1.f)+1.f;
	return roughness_squared/(PI*DENOMINATOR*DENOMINATOR);
}

float schlick_beckmann_geometry(float normal_dot_view, float k)
{ return normal_dot_view/(normal_dot_view*(1-k)+k); }

float smith_schlick_beckmann_geometry(float normal_dot_light,
   float normal_dot_view, float k)
{
   return schlick_beckmann_geometry(normal_dot_view, k)*
      schlick_beckmann_geometry(normal_dot_light, k);
}

vec3 schlick_fresnel(float cosine_theta, vec3 f0)
{ return f0+(1-f0)*pow(1-cosine_theta, 5); }

float shadow_calculation(vec4 position_shadow_space)
{
	const vec2 UV = position_shadow_space.xy*.5f+.5f;
	return position_shadow_space.z-SHADOW_BIAS > texture(shadow_sampler, UV).r ? 0.f : 1.f;
}

void main()
{
	//Skip shading if sky
	const vec3 SKY_COLOR = vec3(.5f, .9f, 3.95296f);
	const float ALBEDO_ALPHA = texture(albedo_sampler, in_uv).a;
	if(ALBEDO_ALPHA < .001f){ out_lighting = vec4(SKY_COLOR, 1); return; }

	//Textures
	const vec3 POSITION = vec4(uniforms.inverse_view*vec4(
		texture(position_sampler, in_uv).xyz, 1.f)).xyz;
	const vec3 NORMAL = normalize(mat3(uniforms.transposed_view)*
		(texture(normal_sampler, in_uv).xyz*2.f-1.f));
	const vec3 ALBEDO = texture(albedo_sampler, in_uv).rgb;
	const float ROUGHNESS = .999f;
	const float ROUGHNESS_SQUARED = .998001f;
	const float ROUGHNESS_PLUS_ONE_SQUARED_DIVIDED_BY_8 = .499500125f;
	const vec4 TRANSLUCENT = texture(translucent_sampler, in_uv);
	const float OCCLUSION = texture(ssao_sampler, in_uv).r;
	const float METALLIC = 0.f;
	const vec3 F0 = mix(vec3(.04f), ALBEDO, METALLIC);

	//Color and light
	const vec3 LIGHT_COLOR = vec3(19.f, 15.f, 9.f);
	const vec3 FOG_COLOR = SKY_COLOR/2.f;
	const vec3 AMBIENT_COLOR = SKY_COLOR/2.f+vec3(.13f);
	const vec3 LIGHT = normalize(vec3(.1f, -1.f, -1.3f));
	const float CAMERA_DISTANCE = distance(uniforms.camera_position.xyz, POSITION);

	//Shadow
		const vec4 POSITION_SHADOW_SPACE = uniforms.shadow_matrix*vec4(POSITION, 1.f);
	float shadow = shadow_calculation(POSITION_SHADOW_SPACE);

	//PBR
	//Vectors
	const vec3 VIEW = normalize(uniforms.camera_position.xyz-POSITION);
	const vec3 HALFWAY = normalize(VIEW+LIGHT);

	//Dot products
	const float NORMAL_DOT_HALFWAY = max(dot(NORMAL, LIGHT), 0);
	const float NORMAL_DOT_LIGHT = max(dot(NORMAL, LIGHT), 0);
	const float NORMAL_DOT_VIEW = max(dot(NORMAL, VIEW), 0);
	const float HALFWAY_DOT_VIEW = max(dot(HALFWAY, VIEW), 0);

	//BRDF
	const float DISTRIBUTION = ggx_distribution(NORMAL_DOT_HALFWAY, ROUGHNESS_SQUARED);
	const float GEOMETRY = smith_schlick_beckmann_geometry(NORMAL_DOT_LIGHT,
		NORMAL_DOT_VIEW, ROUGHNESS_PLUS_ONE_SQUARED_DIVIDED_BY_8);
	const vec3 FRESNEL = schlick_fresnel(HALFWAY_DOT_VIEW, F0);

	const vec3 SPECULAR = DISTRIBUTION*GEOMETRY*FRESNEL/
		max(4*NORMAL_DOT_VIEW*NORMAL_DOT_LIGHT, .001f);
	const vec3 K_DIFFUSE = vec3(1)-FRESNEL*(1-METALLIC);

	const vec3 LIGHTING = (K_DIFFUSE*ALBEDO/PI+SPECULAR)*LIGHT_COLOR*NORMAL_DOT_LIGHT;
	const vec3 AMBIENT = AMBIENT_COLOR*ALBEDO*OCCLUSION;
	const vec3 SHADING = AMBIENT+shadow*LIGHTING;

	//Translucent
	vec3 result;
	const float OPACITY = .8f;
	result = (1-TRANSLUCENT.a*OPACITY)*SHADING+OPACITY*TRANSLUCENT.rgb;

	//Fog
	const float FOG_FADE_START = 200.f;
	const float FOG_FADE_END = 2048.f;
	const float FOG_FADE = clamp((FOG_FADE_END-CAMERA_DISTANCE)/
		(FOG_FADE_END-FOG_FADE_START), 0.f, 1.f);
	result = (1.f-FOG_FADE)*FOG_COLOR+result.rgb*FOG_FADE;

	//Output
	out_lighting = vec4(result, 1);
}