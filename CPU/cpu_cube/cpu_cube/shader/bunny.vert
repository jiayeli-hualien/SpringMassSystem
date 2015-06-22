#version 410 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;

flat out vec4 color;
out vec4 normal;

layout(location = 3) uniform mat4 PVM;

void main()
{
	gl_Position = PVM*vPosition;
	normal = vNormal;
}