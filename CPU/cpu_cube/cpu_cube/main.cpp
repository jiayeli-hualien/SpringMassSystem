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

struct View
{
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
}viewing;


enum VAO_IDs{Cube=0, NumVAOs};
enum EBO_IDs{CUBE_INDEX=0,NumEBOs};
enum Buffer_IDs{ArrayBuffer, NumBuffers};
enum Attrib_IDS{vPosition=0, vColor};

GLuint VAOs[NumVAOs];
GLuint VBOs[NumBuffers];
GLuint EBOs[NumEBOs];

//TODO auto generate
GLfloat cube_positions[]=
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
GLint patition = 16;
GLsizei cube_positions_size = sizeof(cube_positions);

GLfloat cube_colors[] =
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
GLsizei cube_colors_size = sizeof(cube_colors);

/*
static const GLushort RESTART_INDEX=0xFFFF;
static const GLushort cube_indices[] =
{
	0, 1, 2, 3, 6, 7, 4, 5,
	RESTART_INDEX,
	2, 6, 0, 4, 1, 5, 3, 7
};*/

ShaderInfo cubeShaderInfo[]= 
{
	{ GL_VERTEX_SHADER,  "shader\\cube.vert"},
	{ GL_FRAGMENT_SHADER, "shader\\cube.frag"},
	{ GL_NONE, NULL}
};

GLuint cubeShaderProgram;

//print text
//http://stackoverflow.com/questions/2183270/what-is-the-easiest-way-to-print-text-to-screen-in-opengl
/*
GLUT_BITMAP_8_BY_13
GLUT_BITMAP_9_BY_15
GLUT_BITMAP_TIMES_ROMAN_10
GLUT_BITMAP_TIMES_ROMAN_24
GLUT_BITMAP_HELVETICA_10
GLUT_BITMAP_HELVETICA_12
GLUT_BITMAP_HELVETICA_18
*/
void printText(int x, int y, float r, float g, float b, GLvoid* font, char *string)
{
	glColor3f(r, g, b);
	glRasterPos2f(x, y);
	GLsizei len, i;
	len = (GLsizei)strlen(string);
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(font, string[i]);
	}
}

void beginPrintText()
{
	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION_MATRIX);
	glPushMatrix();
	glOrtho(0, windowparam.width, 0, windowparam.height, 0, 100);

}
void endPrintText()
{
	//exit print text
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION_MATRIX);
	glPopMatrix();
}

void display()
{
	puts("Drawing");
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
	glUseProgram(cubeShaderProgram);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER,VBOs[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);

	glm::mat4 projection = glm::perspective(45.0f/180.0f*glm::pi<float>(), (float)windowparam.width/windowparam.height, 0.1f, 100.0f);
	viewing.eye = glm::vec3(2.0f, 2.0f, 2.0f),
	viewing.center = glm::vec3(0.0f, 0.0f, 0.0f),
	viewing.up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAt(viewing.eye, viewing.center, viewing.up);
	glm::mat4 model = glm::mat4(1);
	glm::mat4 PVM = projection*view*model;

	
	glProgramUniformMatrix4fv(cubeShaderProgram, glGetUniformLocation(cubeShaderProgram, "PVM"), 1, GL_FALSE, glm::value_ptr(PVM));
	//glEnable(GL_PRIMITIVE_RESTART);
	//glPrimitiveRestartIndex(RESTART_INDEX);
	//glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
	
	glPointSize(5.0);
	glDrawArrays(GL_POINTS,0,cube_positions_size/sizeof(GLfloat)/4);
	

	//init
	beginPrintText();
	//show text here
	printText(0, windowparam.height-24, 0, 0, 0, GLUT_BITMAP_TIMES_ROMAN_24, "ABCDEFG");

	endPrintText();

	glutSwapBuffers();
}

void initBuffers()
{
	/*glGenBuffers(1, EBOs);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STREAM_DRAW);*/

	glGenVertexArrays(1, VAOs);
	glBindVertexArray(VAOs[0]);

	glGenBuffers(1, VBOs);
	glBindBuffer(GL_ARRAY_BUFFER,VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, cube_positions_size+cube_colors_size, NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cube_positions_size, cube_positions);
	glBufferSubData(GL_ARRAY_BUFFER, cube_positions_size, cube_colors_size, cube_colors);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)sizeof(cube_positions));//offset

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void intitProgram()
{
	cubeShaderProgram = LoadShader(cubeShaderInfo);
}

void init()
{
	glViewport(0, 0, windowparam.width, windowparam.height);
	glMatrixMode(GL_PROJECTION_MATRIX);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	
	glClearColor(0, 0, 0, 1);
	initBuffers();
	intitProgram();
}

void gameLoop()
{
	cout << __FUNCTION__ << endl;

	GLfloat* const vertex=cube_positions;
	GLfloat const GROUND = -1.0f;
	for (int i = 0; i < cube_positions_size/sizeof(GLfloat); i+=4)
	{
		//vertex[i];//x
		vertex[i+1]-=0.1f/100;//y
		if (vertex[i + 1] < GROUND)
			vertex[i + 1] = GROUND;
		//vertex[i + 2];//z
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cube_positions_size, cube_positions);

	glutPostRedisplay();
}

int main(int argc, char * argv[])
{
	windowparam.width = 800;
	windowparam.height = 600;
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowparam.width, windowparam.height);
	//glutInitContextVersion(4, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow(argv[0]);
	glewExperimental = true;
	glewInit();

	init();

	glutDisplayFunc(display);
	glutIdleFunc(gameLoop);
	glutMainLoop();
	return 0;
}