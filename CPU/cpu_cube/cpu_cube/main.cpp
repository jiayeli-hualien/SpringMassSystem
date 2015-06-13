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
GLint PATITION = 16.0f;
GLfloat invPatition = 1.0f / PATITION;
GLfloat patition_dis = 1.0f / PATITION;
GLsizei CUBE_VERTEX_LENGTH = PATITION*PATITION*PATITION*4;//XYZ
GLsizei cube_positions_size = CUBE_VERTEX_LENGTH*sizeof(GLfloat);
GLfloat *cube_positions;
/*GLfloat cube_positions[]=
{
	-1.0f, -1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f
};*/




/*GLfloat cube_colors[] =
{
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f
};*/
GLfloat *cube_colors;
GLsizei cube_colors_size = CUBE_VERTEX_LENGTH*sizeof(GLfloat);
//GLsizei cube_colors_size = sizeof(cube_colors);


/*
static const GLushort RESTART_INDEX=0xFFFF;
static const GLushort cube_indices[] =
{
	0, 1, 2, 3, 6, 7, 4, 5,
	RESTART_INDEX,
	2, 6, 0, 4, 1, 5, 3, 7
};*/

GLfloat *cube_velosity;


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
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
	glUseProgram(cubeShaderProgram);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER,VBOs[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);

	glm::mat4 projection = glm::perspective(45.0f/180.0f*glm::pi<float>(), (float)windowparam.width/windowparam.height, 0.1f, 100.0f);
	viewing.eye = glm::vec3(2.0f, 2.0f, 3.0f),
	viewing.center = glm::vec3(0.0f, 0.0f, 0.0f),
	viewing.up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAt(viewing.eye, viewing.center, viewing.up);
	glm::mat4 model = glm::mat4(1);
	glm::mat4 PVM = projection*view*model;

	
	glProgramUniformMatrix4fv(cubeShaderProgram, glGetUniformLocation(cubeShaderProgram, "PVM"), 1, GL_FALSE, glm::value_ptr(PVM));
	//glEnable(GL_PRIMITIVE_RESTART);
	//glPrimitiveRestartIndex(RESTART_INDEX);
	//glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
	
	glPointSize(20.0);
	glDrawArrays(GL_POINTS,0,cube_positions_size/sizeof(GLfloat)/4);
	

	//init
	beginPrintText();
	//show text here
	printText(0, windowparam.height-24, 0, 0, 0, GLUT_BITMAP_TIMES_ROMAN_24, "ABCDEFG");

	endPrintText();

	glutSwapBuffers();
}

void initCubeVertex()
{
	GLfloat originX = 0.0f;
	GLfloat originY = 1.0f;
	GLfloat originZ = 0.0f;

	cube_positions = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	int index = 0;

	//不管最佳化了 我好累
	for (int i = 0; i < PATITION; i++)//x
	{

		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = (PATITION*PATITION*i + PATITION*j + k) * 4;
				cube_positions[index] = i*patition_dis+originX;//x
				cube_positions[index + 1] = j*patition_dis + originY;//y
				cube_positions[index + 2] = k*patition_dis + originZ;//z
				cube_positions[index + 3] = 1.0f;//w
			}
		}
	}

	cube_colors = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = (PATITION*PATITION*i + PATITION*j + k) * 4;
				cube_colors[index] = i*invPatition;//r
				cube_colors[index + 1] = j*invPatition;//g
				cube_colors[index + 2] = k*invPatition;//b
				cube_colors[index + 3] = 1.0f;//a
			}
		}
	}

	cube_velosity = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = (PATITION*PATITION*i + PATITION*j + k) * 4;//x,y,z,mass
				cube_velosity[index] = 0.0f;
				cube_velosity[index+1] = 0.0f;
				cube_velosity[index+2] = 0.0f;
				cube_velosity[index+3] = 0.0f;
			}
		}
	}
}

void finalization()
{

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
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid * const)cube_positions_size);//offset

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
	initCubeVertex();
	initBuffers();
	intitProgram();
}

struct Float3
{
	float x,y,z;
	Float3 operator- (const Float3 &other) const
	{
		Float3 ans = {this->x-other.x,this->y-other.y,this->z-other.z};
		return ans;
	}
	Float3 operator*(const float &m) const
	{
		Float3 ans = { this->x*m, this->y*m, this->z*m};
		return ans;
	}
	Float3 operator/(const float &d) const
	{
		Float3 ans = { this->x/d, this->y/d, this->z/d };
		return ans;
	}
	Float3& operator+=(const Float3 f2)
	{
		this->x += f2.x;
		this->y += f2.y;
		this->z += f2.z;
		return *this;
	}
	float norm2()
	{
		return sqrt(x*x+y*y+z*z);
	}
};


float frameT = 1.0f/64.0f;
float Gravity = -0.001f*PATITION;//重力太強會壞掉
float springForceK = 3.0f*PATITION;//彈力太小會塌陷，彈力太強會壞掉
float DISTANCE[4] = {0, invPatition, sqrt(2)*invPatition, sqrt(3)*invPatition};
Float3 springForce(const Float3 &p1, const Float3 &p2, float distance)
{

	Float3 force = {0,0,0};
	//f = -kx
	Float3 diff = (p1 - p2);

	if (diff.norm2() == 0.0f)
		return force;

	Float3 dir = diff / diff.norm2();
	force = dir*(-1.0f)*(diff.norm2()-distance)*springForceK;
	return force;
}

Float3 calcForce(int i,int j, int k, int const &index)
{
	Float3 force = {0,0,0};
	Float3 p1 = { cube_positions[index], cube_positions[index+1], cube_positions[index+2] };
	//sprint force
	
	for (int s = -1; s <= 1; s++)//offsetX i
	{
		if (i + s < 0 || i + s >= PATITION)
			continue;
		for (int t = -1; t <= 1; t++)//offset Y i
		{
			if (j + t < 0 || j + t >= PATITION)
				continue;
			for (int u = -1; u <= 1; u++)//
			{
				if (k + u < 0 || k + u >= PATITION)
					continue;
				if (s == 0 && t == 0 && u == 0)
					continue;

				//if (abs(s) + abs(t) + abs(u)>1)
					//continue;

				int indexNN = (PATITION*PATITION*(i+s) + PATITION*(j+t) + k+u) * 4;
				Float3 p2 = { cube_positions[indexNN], cube_positions[indexNN+1], cube_positions[indexNN+2] };
			
				force += springForce(p1,p2,DISTANCE[abs(s)+abs(t)+abs(u)]);
			}
		}
	}
	//gravity
	force.y += Gravity;

	return force;
}

void applyForce()
{

	//only spring force
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = (PATITION*PATITION*i + PATITION*j + k) <<2;//x,y,z,mass
				Float3 force = calcForce(i,j,k,index);
				cube_velosity[index]+=force.x*frameT;
				cube_velosity[index+1]+=force.y*frameT;
				cube_velosity[index+2]+=force.z*frameT;
			}
		}
	}
}

void updatePosition()
{
	GLfloat* const vertex = cube_positions;
	GLfloat const GROUND = 0.0f;
	for (int i = 0; i < cube_positions_size / sizeof(GLfloat); i += 4)
	{
		//vertex[i];//x
		vertex[i] += cube_velosity[i]*frameT;
		vertex[i + 1] += cube_velosity[i + 1]*frameT;
		vertex[i + 2] += cube_velosity[i + 2]*frameT;
		//vertex[i + 2];//z

		if (vertex[i + 1] < GROUND)
		{
			vertex[i + 1] = -vertex[i + 1];
			cube_velosity[i + 1] = -cube_velosity[i + 1];
		}
	}
}

void updateBufferObject()
{

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cube_positions_size, cube_positions);
}

void gameLoop()
{
	applyForce();
	updatePosition();

	updateBufferObject();

	glutPostRedisplay();
}

int main(int argc, char * argv[])
{
	windowparam.width = 800;
	windowparam.height = 600;
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowparam.width, windowparam.height);
	glutInitWindowPosition(0, 0);
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