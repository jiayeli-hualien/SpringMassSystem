/*
簡介
質量彈簧系統做阻尼運動
mass spring system with damping

Project目的:
增強效能、縮短 step time slot。模擬的越精確、力量就能越大而不出錯。
time slot 越小，代表每秒能計算越多張，物體的移動速度就能越快而不出錯。
計算能力越強，模擬所需的時間越短，越方便開發人員除錯。

觀察

問題1. 力道/模擬精度
力道太強 frameTime太長 速度太快
會造成系統的不穩定及毀滅
solution : 正統 frame time 縮短(更細的模擬單位時間要計算更多 frame 更吃效能)、使用更複雜的積分技巧
				增加更多物理constraints
		   非正統 加入阻力、終端速度、減少力道、增加不符合物理特性的 constraints
		   其它(副作用帶來的好處) 加入阻尼

問題2. 彈性碰撞
目前只能彈碰係數降為0 比較安全?
然後取消 back face 的測試??

心得
如果沒有視覺化，根本沒辦法除錯
有著高效能的繪圖能力是對除錯有幫助的

TODO list

OK show points as mesh
OK collision detection, ray to static mesh
give the mesh normal vector
 load obj model, standford bunny
OK damping, 有可能寫錯再查查文獻 (使用上一次的時間點的速度)
	TODO: 將 damp 與 spring force 分離
		  因為要將過強的 damp 改為最多將度降為0
		  但是強力的 spring force 也有加強彈力的功能?

OK? with some bug, Improved Eular Method
AABB accelerlator
openCL after Monday

interpolation model by point info

彈性疲乏

mesh-mesh static colision
mesh-mesh dynamic colision


references:
FREEGLUT
GLEW
GLM
OpenGL® Programming Guide, Eighth Edition. Dave Shreiner et al.
Interactive Computer Graphics, A TOP-DOWN APPROACH WITH SHADER-BASED OPENGL®. Edward Angel et al.

*/
#include<iostream>
#include<cstdio>
#include<cstdlib>

#include<GL\glew.h>
#include<GL\freeglut.h>
#include<glm\gtc\matrix_transform.hpp>
#include<glm\gtc\type_ptr.hpp>

#include"LoadShader.h"
#include"mathHelper.h"


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
GLint PATITION = 16;
GLfloat invPatition = 1.0f / (PATITION);
GLfloat patition_dis = 1.0f / (PATITION);
GLsizei CUBE_VERTEX_LENGTH = PATITION*PATITION*PATITION*4;//XYZ
GLsizei cube_positions_size = CUBE_VERTEX_LENGTH*sizeof(GLfloat);

GLfloat *cube_positions=NULL;
GLfloat *cube_positions_IEM = NULL;//temp storage for IEM

GLfloat *cube_colors=NULL;
GLsizei cube_colors_size = CUBE_VERTEX_LENGTH*sizeof(GLfloat);


static const GLushort RESTART_INDEX=0xFFFF;
static const GLushort cube_indices[] =
{
	0, 1, 2, 3, 6, 7, 4, 5,
	RESTART_INDEX,
	2, 6, 0, 4, 1, 5, 3, 7
};

GLuint RESTART_INDEX_UINT32 = 0xFFFFFFFF;
GLuint *cube_mesh_index = NULL;
GLsizei cube_mesh_index_length = ((2*PATITION+1)*(PATITION-1))*6*2;//QUAD_STRIP?
GLsizei cube_mesh_index_used_length = 0;
GLsizei cube_mesh_index_size = 0;
//draw element
//QUAD strip,
//0 2 4 ... RESTART 2*Patition+1
//
//1 3 5 ...
//
//v-v-v...  *Patition-1
//6 face


GLfloat *cube_velosity=NULL;
GLfloat *cube_velosity_IEM=NULL;//temp storage
GLfloat *cube_force=NULL;
GLfloat *cube_force_IEM = NULL;//temp storage


ShaderInfo cubeShaderInfo[]= 
{
	{ GL_VERTEX_SHADER,  "shader\\cube.vert"},
	{ GL_FRAGMENT_SHADER, "shader\\cube.frag"},
	{ GL_NONE, NULL}
};

ShaderInfo bunnyShaderInfo[]=
{
	{GL_VERTEX_SHADER, "shader\\bunny.vert"},
	{GL_FRAGMENT_SHADER, "shader\\bunny.frag"},
	{GL_NONE, NULL}
};

GLuint cubeShaderProgram;
GLuint bunnyShaderProgram;

inline int GET_INDEX(int x, int y, int z)
{
	return (PATITION*PATITION*(x) + PATITION*(y) + z) * 4;
}

inline int GET_VERTEX_INDEX(int x, int y, int z)
{
	return (PATITION*PATITION*(x)+PATITION*(y)+z);
}

int time_stamp=0;
int frameCount=0;
GLfloat FPS = 0.0f;
void updateFPS();
void printText(int x, int y, float r, float g, float b, GLvoid* font, char *string);
void beginPrintText();
void endPrintText();
void keyAction(unsigned char key, int x, int y);
void updateBufferObject();

int drawMode = 1;
void display();
void initCubeVertex();

void initCubeMesh();

void initBuffers();
void intitProgram();
void init();



const float DEFAULT_FRAME_T = 1.0 /  64.0f;
float frameT = DEFAULT_FRAME_T;//step time, frame time
float Gravity = -0.00025f*PATITION;//重力太強會壞掉
float springForceK = 20.0f * Gravity*PATITION*PATITION;//彈力太小會塌陷，彈力太強會壞掉
float springDampK = 0.5f;//Hooke's law, K
const bool useDamp = true;
float DISTANCE[4] = {0, invPatition, sqrt(2)*invPatition, sqrt(3)*invPatition};
//float DISTANCE[4] = { 0, invPatition, invPatition, invPatition };
Float3 springForce(const Float3 &p1, const Float3 &p2, const Float3 &v1, const Float3 &v2, float distance);
Float3 calcSpringForce(int i, int j, int k, Float3 &p1, Float3 &v1, GLfloat *positions, GLfloat *velosity);
void applyForce(GLfloat *force, GLfloat *velositySrc, GLfloat *velosityDest);//v = at
void updateForce(GLfloat *positions, GLfloat *velosity, GLfloat *force);// g = g(u(t), t)   u is position & velosity
void updatePosition(GLfloat *velosity, GLfloat *positionsSrc, GLfloat *positionsDst);//x=vt and collision detection
void applyForceIEM(GLfloat *force, GLfloat *force2, GLfloat *velosity);//a = 

const int EULAR_METHOD = 0;
const int IMPROVED_EULAR_METHOD = 1;
int INTEGRATION = IMPROVED_EULAR_METHOD;
void gameLoop();

#include "LoadObj.h"
LoadObj modelBunny;
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

	glutKeyboardFunc(keyAction);
	glutDisplayFunc(display);
	glutIdleFunc(gameLoop);
	glutMainLoop();
	return 0;
}
void updateFPS()
{
	if (time_stamp == 0)
	{
		time_stamp = glutGet(GLUT_ELAPSED_TIME);
		return;
	}

	int updatetime = glutGet(GLUT_ELAPSED_TIME);//miliscesond
	frameCount++;

	if (updatetime - time_stamp > 1000)
	{
		FPS = (float)frameCount / (float)(updatetime - time_stamp) * 1000;
		time_stamp = updatetime;
		frameCount = 0;
	}
}

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
	updateFPS();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glUseProgram(cubeShaderProgram);
	glEnable(GL_DEPTH_TEST);
	
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);


	glm::mat4 projection = glm::perspective(45.0f / 180.0f*glm::pi<float>(),
		(float)windowparam.width / windowparam.height, 0.1f, 100.0f);
	viewing.eye = glm::vec3(2.0f, 2.0f, 3.0f),
		viewing.center = glm::vec3(0.0f, 0.0f, 0.0f),
		viewing.up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAt(viewing.eye, viewing.center, viewing.up);
	glm::mat4 model = glm::mat4(1);
	glm::mat4 PVM = projection*view*model;

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid * const)cube_positions_size);//offset
	glProgramUniformMatrix4fv(cubeShaderProgram,
		glGetUniformLocation(cubeShaderProgram, "PVM"),
		1, GL_FALSE, glm::value_ptr(PVM));

	glEnable(GL_PRIMITIVE_RESTART);
	//glPrimitiveRestartIndex(RESTART_INDEX);
	glPrimitiveRestartIndex(RESTART_INDEX_UINT32);
	if (drawMode == 1 || drawMode == 2)
	{
		//glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
		glDrawElements(GL_QUAD_STRIP, cube_mesh_index_used_length, GL_UNSIGNED_INT, 0);

	}


	//glPointSize(20.0);
	glPointSize(10.0);
	if (drawMode == 0 || drawMode == 2)
		glDrawArrays(GL_POINTS, 0, cube_positions_size / sizeof(GLfloat) / 4);

	glUseProgram(bunnyShaderProgram);
	glProgramUniformMatrix4fv(cubeShaderProgram,
		glGetUniformLocation(cubeShaderProgram, "PVM"),
		1, GL_FALSE, glm::value_ptr(PVM));
	modelBunny.draw();






	





	//init
	beginPrintText();
	//show text here
	char buffer[256];
	sprintf_s(buffer, 256, "FPS %f", FPS);
	printText(0, windowparam.height - 24, 0, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24, buffer);
	sprintf_s(buffer, 256, "PATITION : %dx%dx%d", PATITION, PATITION, PATITION);
	printText(0, windowparam.height - 48, 0, 0, 1, GLUT_BITMAP_TIMES_ROMAN_24, buffer);
	endPrintText();

	glutSwapBuffers();
}

void initCubeVertex()
{
	float transX=0.0f;
	float transY=1.0f;
	float transZ=0.0f;
	float rotate = 0.0f;

	glm::mat4x4 modelMatrix;
	


	modelMatrix = glm::translate(modelMatrix, glm::vec3(transX, transY, transZ));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
	modelMatrix = glm::rotate(modelMatrix, rotate / 180.0f*glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, -0.5f, -0.5f));

	cube_positions = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	cube_positions_IEM = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	int index = 0;

	//不管最佳化了 我好累
	for (int i = 0; i < PATITION; i++)//x
	{

		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);
				cube_positions[index] = i*patition_dis;//x
				cube_positions[index + 1] = j*patition_dis ;//y
				cube_positions[index + 2] = k*patition_dis ;//z
				cube_positions[index + 3] = 1.0f;//w for homogeneous system


				glm::vec4 position(cube_positions[index], cube_positions[index+1], cube_positions[index+2], cube_positions[index+3]);

				position = modelMatrix*position;

				cube_positions[index] = position.x / position.w;
				cube_positions[index + 1] = position.y / position.w;
				cube_positions[index + 2] = position.z / position.w;
				cube_positions[index + 3] = position.w / position.w;


				cube_positions_IEM[index + 3] = 1.0f;//w for homogeneous system
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
				int index = GET_INDEX(i, j, k);
				cube_colors[index] = i*invPatition;//r
				cube_colors[index + 1] = j*invPatition;//g
				cube_colors[index + 2] = k*invPatition;//b
				cube_colors[index + 3] = 1.0f;//a

			}
		}
	}

	cube_velosity = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	cube_velosity_IEM = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);//x,y,z,mass
				cube_velosity[index] = 0.0f;
				cube_velosity[index + 1] = 0.0f;
				cube_velosity[index + 2] = 0.0f;
				cube_velosity[index + 3] = 1.0f;//weight, mass

				cube_velosity_IEM[index + 3] = 1.0f;
			}
		}
	}

	cube_force = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	cube_force_IEM = new GLfloat[CUBE_VERTEX_LENGTH * 4];
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);//x,y,z,mass
				cube_force[index] = 0;
				cube_force[index + 1] = 0;
				cube_force[index + 2] = 0;
				cube_force[index + 3] = 0;

				cube_force_IEM[index + 3] = 0;
			}
		}
	}
}

//TODO 特別法向量的vertex
void initCubeMesh()
{
	cube_mesh_index = new GLuint[cube_mesh_index_length];

	// six faces

	int top = 0;
	int x = 0, y = 0, z = 0;

	//UP DOWN
	int roundV[2] = { 0, PATITION - 1 };
	for (int round = 0; round < 2; round++)
	{
		y = roundV[round];
		for (int i = 0; i < PATITION - 1; i++)
		{
			z = i;
			for (int j = 0; j < PATITION; j++)
			{
				x = j;

				cube_mesh_index[top++] = GET_VERTEX_INDEX(x, y, z + 1);
				cube_mesh_index[top++] = GET_VERTEX_INDEX(x, y, z);
			}
			//restart
			cube_mesh_index[top++] = RESTART_INDEX_UINT32;
		}
	}
	//left right
	for (int round = 0; round < 2; round++)
	{
		x = roundV[round];
		for (int i = 0; i < PATITION - 1; i++)
		{
			y = i;
			for (int j = 0; j < PATITION; j++)
			{
				z = j;
				cube_mesh_index[top++] = GET_VERTEX_INDEX(x, y, z);
				cube_mesh_index[top++] = GET_VERTEX_INDEX(x, y + 1, z);
			}
			//restart
			cube_mesh_index[top++] = RESTART_INDEX_UINT32;
		}
	}

	//frount back
	for (int round = 0; round < 2; round++)
	{
		z = roundV[round];
		for (int i = 0; i < PATITION - 1; i++)
		{
			x = i;
			for (int j = 0; j < PATITION; j++)
			{
				y = j;
				cube_mesh_index[top++] = GET_VERTEX_INDEX(x, y, z);
				cube_mesh_index[top++] = GET_VERTEX_INDEX(x + 1, y, z);
			}
			//restart
			cube_mesh_index[top++] = RESTART_INDEX_UINT32;
		}
	}
	cube_mesh_index_used_length = top;
	cube_mesh_index_size = top*sizeof(GLuint);
}


void initBuffers()
{
	//glGenBuffers(1, EBOs);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
	glGenBuffers(1, EBOs);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_mesh_index_size, cube_mesh_index, GL_STATIC_DRAW);


	glGenVertexArrays(1, VAOs);
	glBindVertexArray(VAOs[0]);

	glGenBuffers(1, VBOs);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, cube_positions_size + cube_colors_size, NULL, GL_STREAM_DRAW);
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
	bunnyShaderProgram = LoadShader(bunnyShaderInfo);
}


bool loadBunny = true;
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
	initCubeMesh();
	initBuffers();
	if (loadBunny)
	{
		//modelBunny.load("model\\bunny.obj");
		modelBunny.load("model\\triangle.obj");
		modelBunny.initGL_Buffer();
	}
	intitProgram();
}

Float3 springForce(const Float3 &p1, const Float3 &p2, const Float3 &v1, const Float3 &v2, float distance)
{

	Float3 force = { 0, 0, 0 };
	//f = -kx
	Float3 diff = (p1 - p2);
	Float3 diffV = (v1 - v2);
	if (diff.norm2() == 0.0f)
		return force;

	Float3 dir = diff / diff.norm2();
	const float springForce = (diff.norm2() - distance)*springForceK;

	if (useDamp)
	{
		float sprintDamping = diffV.dot(dir)*springDampK;
		force = dir*(springForce - sprintDamping);
	}
	else
		force = force = dir*(-1.0f)*(springForce);

	return force;
}

Float3 calcSpringForce(int i, int j, int k, Float3 &p1, Float3 &v1, GLfloat *positions, GLfloat *velosity)
{
	Float3 force = { 0, 0, 0 };
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

				//if (abs(s) + abs(t) + abs(u)>2) continue;//only 18 NN

				//if (abs(s) + abs(t) + abs(u)>1) continue;//only 6 NN



				int indexNN = GET_INDEX(i + s, j + t, k + u);
				Float3 p2 = { positions[indexNN], positions[indexNN + 1], positions[indexNN + 2] };
				Float3 v2 = { velosity[indexNN], velosity[indexNN + 1], velosity[indexNN + 2] };

				force += springForce(p1, p2, v1, v2, DISTANCE[abs(s) + abs(t) + abs(u)]);
			}
		}
	}
	//gravity
	force.y += Gravity;

	return force;
}

void applyForce(GLfloat *force, GLfloat *velositySrc, GLfloat *velosityDest)
{

	//only spring force
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);//x,y,z,mass
				velosityDest[index + 3] = velositySrc[index + 3];//copy mass
				velosityDest[index] = velositySrc[index] + force[index] * frameT / velositySrc[index + 3];
				velosityDest[index + 1] = velositySrc[index + 1] + force[index + 1] * frameT / velositySrc[index + 3];
				velosityDest[index + 2] = velositySrc[index + 2] + force[index + 2] * frameT / velositySrc[index + 3];
			}
		}
	}
}


void updateForce(GLfloat *positions, GLfloat *velosity, GLfloat *force)
{
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);//x,y,z,mass
				Float3 p1 = { positions[index], positions[index + 1], positions[index + 2] };
				Float3 v1 = { velosity[index], velosity[index + 1], velosity[index + 2] };
				Float3 springF = calcSpringForce(i, j, k, p1, v1, positions, velosity);
				force[index] = springF.x;
				force[index + 1] = springF.y + velosity[index + 3] * Gravity;
				force[index + 2] = springF.z;
			}
		}
	}
}

bool testIntersection = false;
bool testIntersectionModel = true;
void updatePosition(GLfloat *velosity, GLfloat *positionsSrc, GLfloat *positionsDst, bool enable_collision_detection)
{
	GLfloat* const vertex = positionsDst;
	GLfloat const GROUND = -0.1f;

	//test triangle palne
	Float3 v0{ -0.1f, 0.5f, -0.1f }, v1{ -0.1f, 0.5f, 1.0f }, v2{ 1.0f, 0.5f, 1.0f };


	for (int i = 0; i < cube_positions_size / sizeof(GLfloat); i += 4)
	{
		Float3 O = { positionsSrc[i], positionsSrc[i+1], positionsSrc[i+2] };
		vertex[i] = positionsSrc[i] + velosity[i] * frameT;
		vertex[i + 1] = positionsSrc[i + 1] + velosity[i + 1] * frameT;
		vertex[i + 2] = positionsSrc[i + 2] + velosity[i + 2] * frameT;

		if (!enable_collision_detection)
		{
			continue;
		}

		if (testIntersection)
		{

			//0.5 triangle plane test
			Float3 diff = { vertex[i], vertex[i + 1], vertex[i + 2] };
			diff = diff - O;
			float t = 0.0f, u = 0.0f, v = 0.0f;


			if (intersect_triangle(O, diff, v0, v1, v2, &t, &u, &v) != 0)
			if (t <= 1.0f)
			{
				//vertex[i + 1] = 0.5f + 2 * (0.5f - vertex[i + 1]);
				//vertex[i+1] = 0.5f;//座標別改了 Q_Q
				velosity[i + 1] = abs(velosity[i + 1]);//base on normal  內積normal之後 扣掉再加與normal同向?


				//velosity[i] = 0.0f;
				//velosity[i + 2] = 0.0f;
			}
		}

		if (testIntersectionModel)
		{
			//0.5 triangle plane test
			for (int j = 0; j<modelBunny.n_face; j+=1)
			{
				int v0Index = modelBunny.faces[j*3];
				int v1Index = modelBunny.faces[j*3 + 1];
				int v2Index = modelBunny.faces[j*3 + 2];

				Float3 v0{ modelBunny.vertex[v0Index * 4],
					modelBunny.vertex[v0Index * 4 + 1],
					modelBunny.vertex[v0Index * 4 + 2] };

				Float3 v1{ modelBunny.vertex[v1Index * 4],
					modelBunny.vertex[v1Index * 4 + 1],
					modelBunny.vertex[v1Index * 4 + 2] };

				Float3 v2{
					modelBunny.vertex[v2Index * 4],
					modelBunny.vertex[v2Index * 4 + 1],
					modelBunny.vertex[v2Index * 4 + 2]
				};

				Float3 diff = { vertex[i], vertex[i + 1], vertex[i + 2] };
				diff = diff - O;
				float t = 0.0f, u = 0.0f, v = 0.0f;


				if (intersect_triangle(O, diff, v0, v1, v2, &t, &u, &v) != 0)
				if (t <= 1.0f)
				{
					vertex[i + 1] = abs(vertex[i + 1]);
					//vertex[i+1] = 0.0f;//座標別改了 Q_Q
					velosity[i + 1] = abs(velosity[i + 1]);//base on normal  內積normal之後 扣掉再加與normal同向?
					velosity[i + 1] = 0.0f;

					//velosity[i] = 0.0f;
					//velosity[i + 2] = 0.0f;
				}
			}
			continue;
		}

		
		if (vertex[i + 1] <= GROUND)
		{
		vertex[i + 1] = -vertex[i + 1];
		velosity[i + 1] = -velosity[i + 1];


		//摩擦力
		velosity[i] = 0.0f;
		velosity[i + 2] = 0.0f;
		}
	}
}


void applyForceIEM(GLfloat *force, GLfloat *force2, GLfloat *velosity)
{
	for (int i = 0; i < PATITION; i++)//x
	{
		for (int j = 0; j < PATITION; j++)//y
		{
			for (int k = 0; k < PATITION; k++)//z
			{
				int index = GET_INDEX(i, j, k);//x,y,z,mass

				velosity[index] += 0.5f*(force[index] + force2[index])* frameT / velosity[index + 3];
				velosity[index + 1] += 0.5f*(force[index + 1] + force2[index + 1]) * frameT / velosity[index + 3];
				velosity[index + 2] += 0.5f*(force[index + 2] + force2[index + 2]) * frameT / velosity[index + 3];
			}
		}
	}
}


void gameLoop()
{
	//for (int i = 0; i < cube_positions_size / 4; i += 4)
	//{
	//	printf("%f %f %f %f \n", cube_positions[i], cube_positions[i + 1], cube_positions[i + 2], cube_positions[i + 3]);
	//}
	if (INTEGRATION == EULAR_METHOD)
	{
		updateForce(cube_positions, cube_velosity, cube_force);
		applyForce(cube_force, cube_velosity, cube_velosity);
		updatePosition(cube_velosity, cube_positions, cube_positions, true);
	}
	else
	if (INTEGRATION == IMPROVED_EULAR_METHOD)
	{
		updateForce(cube_positions, cube_velosity, cube_force);//g(u(t),t) in cube_force
		applyForce(cube_force, cube_velosity, cube_velosity_IEM);//vx vy vz of u(t+h) in cube_velosity_IEM
		updatePosition(cube_velosity_IEM, cube_positions, cube_positions_IEM, false);//x,y,z of u(t+h) in  cube_positions_IEM

		//u(t) in cube_positions & cube_velosity

		//calc g(u(t+h), t+h)
		updateForce(cube_positions_IEM, cube_velosity_IEM, cube_force_IEM);
		applyForceIEM(cube_force_IEM, cube_force, cube_velosity);
		updatePosition(cube_velosity, cube_positions, cube_positions, true);

	}


	updateBufferObject();
	glutPostRedisplay();
}

void updateBufferObject()
{

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cube_positions_size, cube_positions);
}




void keyAction(unsigned char key, int x, int y)
{
	printf("press %c\n", key);

	if (key == '0')
	{
		drawMode = 0;
	}
	else
	if (key == '1')
	{
		drawMode = 1;
	}
	else
	if (key == '2')
	{
		drawMode = 2;
	}
	if (key == 27)
	{
		exit(0);
	}
}