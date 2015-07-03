#version 410 core
out vec4 fColor;
in vec4 normal;

void main()
{
	float lumin = (normal.x+normal.y+normal.z)*0.6;
	if(lumin<0.0f)
		lumin = 0.0f;
	if(lumin>1.0f)
		lumin = 1.0f;
	fColor = vec4(0.1,0.1,0.1,1.0)+vec4(0.9,0.9,0.9,0.0)*lumin;
}