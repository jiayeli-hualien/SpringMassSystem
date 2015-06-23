#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<string>
#include<fstream>
#include<sstream>

#include<GL\glew.h>
#include<GL\freeglut.h>
#include<glm\gtc\matrix_transform.hpp>

struct ShaderInfo
{
	GLenum target;
	char *fileName;
};

GLuint LoadShader(ShaderInfo *shaderinfo);

bool readShaderFile(const char * fileName, std::string &str);