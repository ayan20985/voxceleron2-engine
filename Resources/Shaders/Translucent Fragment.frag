#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 fragment_color;

void main(){ fragment_color =  vec4(.05f, .1f, .6f, 1.f); }