#version 410 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;

out vec4 color;

layout(location = 3) uniform mat4 PVM;

void main()
{
	gl_Position = PVM*vPosition;
	color = vColor;	
}