#include"LoadShader.h"
using namespace std;
GLuint LoadShader(ShaderInfo *shaderinfo){
	
	GLuint program;

	program = glCreateProgram();
	glUseProgram(program);

	while (shaderinfo->target)
	{
		GLuint shader = glCreateShader(shaderinfo->target);
		string textStr;
		readShaderFile(shaderinfo->fileName, textStr);
		const char *text = textStr.c_str();
		glShaderSource(shader, 1, &text, NULL);

		cout << "shader source:" << endl;
		cout << text << endl;

		glCompileShader(shader);
		GLint status;

		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

		if (status != GL_TRUE)
		{
			cerr << "compile error" << endl;
		}

		GLchar *infoLog;
		GLsizei buffLength = 0;
		glGetShaderInfoLog(shader, NULL, &buffLength, NULL);
		infoLog = new GLchar[buffLength+1];
		infoLog[0] = '\0';
		glGetShaderInfoLog(shader, buffLength, &buffLength, infoLog);
		cout << "infolog :" << infoLog << endl;

		delete[] infoLog;

		shaderinfo++;
	}

	glLinkProgram(program);


	return program;
};

bool readShaderFile(const char * fileName, string &str){
	ifstream fin;
	fin.open(fileName);

	if (!fin.is_open())
	{
		cerr << "Can not open file :" << fileName << endl;
		return false;
	}
	
	stringstream buffer;
	buffer << fin.rdbuf();
	str = buffer.str();
	buffer.clear();
	fin.close();
	return true;
};