#include<iostream>
#include<cstdio>
#include<cstdlib>

#include<GL\glew.h>
#include<GL\freeglut.h>
#include<glm\gtc\matrix_transform.hpp>
#include<glm\gtc\type_ptr.hpp>

#include"LoadShader.h"

using namespace std;

struct WindowParam
{
	int width;
	int height;
}windowparam;


enum VAO_IDs{Cube=0, NumVAOs};
enum EBO_IDs{CUBE_INDEX=0,NumEBOs};
enum Buffer_IDs{ArrayBuffer, NumBuffers};
enum Attrib_IDS{vPosition=0, vColor};

GLuint VAOs[NumVAOs];
GLuint VBOs[NumBuffers];
GLuint EBOs[NumEBOs];

static const GLfloat cube_positions[]=
{
	-1.0f, -1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};

static const GLfloat cube_colors[] =
{
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f
};

static const GLushort RESTART_INDEX=0xFFFF;
static const GLushort cube_indices[] =
{
	0, 1, 2, 3, 6, 7, 4, 5,
	RESTART_INDEX,
	2, 6, 0, 4, 1, 5, 3, 7
};

ShaderInfo cubeShaderInfo[]= 
{
	{ GL_VERTEX_SHADER,  "shader\\cube.vert"},
	{ GL_FRAGMENT_SHADER, "shader\\cube.frag"},
	{ GL_NONE, NULL}
};

GLuint cubeShaderProgram;


void display()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	

	glUseProgram(cubeShaderProgram);
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);

	glm::mat4 projection = glm::perspective(45.0f/180.0f*glm::pi<float>(), (float)windowparam.width/windowparam.height, 0.1f, 100.0f);
	glm::vec3 eye = glm::vec3(2.0f, 2.0f, 2.0f),
		center = glm::vec3(0.0f, 0.0f, 0.0f),
		up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAt(eye, center, up);
	glm::mat4 model = glm::mat4();
	glm::mat4 PVM = projection*view*model;

	glProgramUniformMatrix4fv(cubeShaderProgram, glGetUniformLocation(cubeShaderProgram, "PVM"), 1, GL_FALSE, glm::value_ptr(PVM));

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(RESTART_INDEX);
	glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);

	glutSwapBuffers();
}

void initBuffers()
{
	glGenBuffers(1, EBOs);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STREAM_DRAW);

	glGenVertexArrays(1, VAOs);
	glBindVertexArray(VAOs[0]);

	glGenBuffers(1, VBOs);
	glBindBuffer(GL_ARRAY_BUFFER,VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_positions)+sizeof(cube_colors), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_positions), cube_positions);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(cube_positions), sizeof(cube_colors), cube_colors);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)sizeof(cube_positions));//offset
}

void intitProgram()
{
	cubeShaderProgram = LoadShader(cubeShaderInfo);
}

void init()
{
	glViewport(0,0,windowparam.width, windowparam.height);
	glEnable(GL_DEPTH_TEST);
	initBuffers();

	intitProgram();
}

int main(int argc, char * argv[])
{
	windowparam.height = 600;
	windowparam.width = 800;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowparam.width, windowparam.height);
	glutInitContextVersion(4, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow(argv[0]);
	glewExperimental = true;
	glewInit();

	init();

	glutDisplayFunc(display);
	glutMainLoop();
	return 0;
}