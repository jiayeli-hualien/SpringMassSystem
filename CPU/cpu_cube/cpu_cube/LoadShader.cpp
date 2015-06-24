#include"LoadShader.h"
using namespace std;
GLuint LoadShader(ShaderInfo *shaderinfo){
	
	GLuint program;

	program = glCreateProgram();


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
		GLsizei buffLength = 5;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &buffLength);
		infoLog = new GLchar[buffLength+1];
		infoLog[0] = '\0';
		glGetShaderInfoLog(shader, buffLength, &buffLength, infoLog);
		cout << "infolog :" << infoLog << endl;

		delete[] infoLog;
		glAttachShader(program, shader);
		shaderinfo++;
	}

	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		cerr << "Link fail" << endl;

		GLchar *infoLog = NULL;
		GLsizei buffLength = 0;
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &buffLength);
		infoLog = new GLchar[buffLength + 1];
		infoLog[0] = '\0';
		glGetProgramInfoLog(program, buffLength, &buffLength, infoLog);
		cout << "infolog :" << infoLog << endl;

		delete[] infoLog;
	}

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