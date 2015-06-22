#version 410 core
out vec4 fColor;
flat in vec4 color;
in vec4 normal;

void main()
{
	fColor = vec4(0.2,0.2,0.2,0.2)+vec4(0.5,0.5,0.5,0.5)*(normal.x+normal.y+normal.z);
}