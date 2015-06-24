#version 410 core
#extension GL_ARB_explicit_attrib_location : enable
//#extension GL_ARB_explicit_uniform_location : enable

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;

flat out vec4 color;
out vec4 normal;

uniform mat4 PVM;

void main()
{
	gl_Position = PVM*vPosition;
	normal = vNormal;
}