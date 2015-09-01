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
OK load obj model, standford bunny
OK damping, 有可能寫錯再查查文獻 (使用上一次的時間點的速度)
	TODO: 將 damp 與 spring force 分離
		  因為要將過強的 damp 改為最多將度降為0
		  但是強力的 spring force 也有加強彈力的功能?

OK? with some bug, Improved Eular Method
AABB accelerlator
OK OpenCL GPU, CPU for eular method
OpenGL memory range map

interpolation model by point info

彈性疲乏

mesh-mesh static colision
mesh-mesh dynamic colision


references:
1.	Dave Shreiner, Graham Sellers, John M. Kessenich, Bill M. Licea-Kane. OpenGL® Programming Guide: The Official Guide to Learning OpenGL, Version 4.3 (8th Edition). Pearson Education. 2013.
2.	Edward Angel, Dave Shreiner. Interactive Computer Graphics: A TOP-DOWN APPROACH WITH SHADER-BASED OPENGL®. Pearson, 2011.
3.	FreeGLUT http://freeglut.sourceforge.net/
4.	GLEW - The OpenGL Extension Wrangler Library. http://glew.sourceforge.net/
5.	GLM - OpenGL Mathematics. http://glm.g-truc.net/0.9.6/index.html
6.	Mike Bailey. OpenCL / OpenGL Vertex Buffer Interoperability: A Particle System Case Study.
7.	Ravishekhar Banger, Koushik Bjattacharyya. OpenCL Programming by Example. Packt Publishing. 2013.
8.	Tomas M¨oller and Ben Trumbore. Fast, minimum storage ray-triangle intersection. Journal of Graphics Tools, 2(1):21–28, 1997.

*/
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<math.h>
#include<GL\glew.h>
#include<GL\freeglut.h>
#include<glm\gtc\matrix_transform.hpp>
#include<glm\gtc\type_ptr.hpp>

#include<CL\cl.h>
#include<CL\cl_gl.h>

using namespace std;


#include"LoadShader.h"
#include"mathHelper.h"
#include"vsync.h"
#include"testOpenCL.h"



//TODO auto generate
GLint PATITION = 16;
GLfloat invPatition = 1.0f / (PATITION-1.0f);
GLfloat patition_dis = 1.0f / (PATITION-1.0f);

const float DEFAULT_FRAME_T = 1.0 / 64.0f;
bool enableVsync = false;//限制60 FPS

float Gravity = -0.01f/PATITION*10*10;//重力太強會壞掉
float springForceK = abs(10.0f * Gravity*PATITION*PATITION);//彈力太小會塌陷，彈力太強會壞掉
float springDampK = 0.5f;//Hooke's law, K

const int EULAR_METHOD = 0;
const int IMPROVED_EULAR_METHOD = 1;
int INTEGRATION = IMPROVED_EULAR_METHOD;

const int NAIVE_CPU_UPDATE = 0;
const int CL_GPU_UPDATE = 1;
const int CL_CPU_UPDATE = 2;
int UPDATE_METHOD = NAIVE_CPU_UPDATE;
//int UPDATE_METHOD = CL_GPU_UPDATE;
//int UPDATE_METHOD = CL_CPU_UPDATE;

bool loadColiisionModel = true;
bool loadDefomModel = true;

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

inline int GET_INDEX(const int x, const int y, const int z)
{
	return (PATITION*PATITION*(x) + PATITION*(y) + z) * 4;
}

inline int GET_VERTEX_INDEX(const int x, const int y, const int z)
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

int drawMode = 3;
void display();
void initCubeVertex();

void initCubeMesh();

void initBuffers();
void intiShaderProgram();
void init();



float frameT = DEFAULT_FRAME_T;//step time, frame time

float DISTANCE[4] = { 0, patition_dis, sqrt(2)*patition_dis, sqrt(3)*patition_dis};
Float3 springForce(const Float3 &p1, const Float3 &p2, const Float3 &v1, const Float3 &v2, float distance);
Float3 calcSpringForce(int i, int j, int k, Float3 &p1, Float3 &v1, GLfloat *positions, GLfloat *velosity);
void applyForce(GLfloat *force, GLfloat *velositySrc, GLfloat *velosityDest);//v = at
void updateForce(GLfloat *positions, GLfloat *velosity, GLfloat *force);// g = g(u(t), t)   u is position & velosity


float BOUNCE_COEF = 1.0f;
void updatePosition(GLfloat *velosity, GLfloat *positionsSrc, GLfloat *positionsDst);//x=vt and collision detection
void applyForceIEM(GLfloat *force, GLfloat *force2, GLfloat *velosity);//a = 


char *loadCLKernelFile(char* fileName, size_t *retStringSize);
//http://web.engr.oregonstate.edu/~mjb/cs475/Handouts/opencl.opengl.vbo.6pp.pdf
namespace UseCL{

	//TODO 3D ND Range, PATITION^3

	cl_platform_id *platforms = NULL;
	cl_uint num_platforms;
	cl_uint usePlatform = 1;

	cl_device_id *device_list = NULL;
	cl_uint num_devices;
	cl_context context = 0;

	cl_command_queue command_queue = 0;

	cl_program program_updateForce;
	cl_kernel kernel_updateForce;

	cl_program program_applyForce;
	cl_kernel kernel_applyForce;

	cl_program program_updatePosition;
	cl_kernel kernel_updatePosition;

	cl_program program_applyForceIEM;
	cl_kernel kernel_applyForceIEM;

	size_t global_size[3];
	size_t local_size[3] = { glm::min((unsigned int)4, (unsigned int)PATITION),
		glm::min((unsigned int)4, (unsigned int)PATITION),
		glm::min((unsigned int)4, (unsigned int)PATITION) };

	cl_mem position_clmem;
	cl_mem velosity_clmem;
	cl_mem force_clmem;
	cl_mem position_swap_clmem;//for IEM
	cl_mem velosity_swap_clmem;
	cl_mem force_swap_clmem;

	bool isCLGLInteropSupported(char* extensionString);
	void initCL();
	void calcSpringForceCL();
	void clUpdate();
}




bool enableUpdate = true;



void cpuUpdate();
void gameLoop();







#include "LoadObj.h"
LoadObj modelCollision;
DeformObj deformModel;
bool testSAXPY_opencl = false;
int main(int argc, char * argv[])
{
	if (testSAXPY_opencl)
		SAXPY::testOpenCL();



	windowparam.width = 800;
	windowparam.height = 600;
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowparam.width, windowparam.height);
	glutInitWindowPosition(0, 0);
	glutInitContextVersion(3, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);


	glutCreateWindow(argv[0]);

	glewExperimental = true;
	GLuint success = glewInit();

	init();

	glutKeyboardFunc(keyAction);
	glutDisplayFunc(display);
	glutIdleFunc(gameLoop);

	glutMainLoop();
	return 0;
}

float recordFPS[20];
int topFPS = 0;
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

		if (topFPS<20)
			recordFPS[topFPS++] = FPS;
		if (topFPS == 20)
		{
			float sum = 0.0f;
			for (int i = 10; i < topFPS; i++)
			{
				sum += recordFPS[i];
			}
			printf("%f\n",sum/10.0f);
			topFPS++;
		}
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
	glPointSize(5.0);
	if (drawMode == 0 || drawMode == 2 || drawMode==4)
		glDrawArrays(GL_POINTS, 0, cube_positions_size / sizeof(GLfloat) / 4);

	glUseProgram(bunnyShaderProgram);
	glProgramUniformMatrix4fv(bunnyShaderProgram,
		glGetUniformLocation(bunnyShaderProgram, "PVM"),
		1, GL_FALSE, glm::value_ptr(PVM));
	if (loadColiisionModel)
		modelCollision.draw();

	if (drawMode==3||drawMode==4)
		if (loadDefomModel)
			deformModel.draw();

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
				cube_positions[index + 3] = 1.0f;


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
	glBufferData(GL_ARRAY_BUFFER, cube_positions_size + cube_colors_size, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, cube_positions_size, cube_positions);
	glBufferSubData(GL_ARRAY_BUFFER, cube_positions_size, cube_colors_size, cube_colors);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid * const)cube_positions_size);//offset


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void intiShaderProgram()
{
	cubeShaderProgram = LoadShader(cubeShaderInfo);
	bunnyShaderProgram = LoadShader(bunnyShaderInfo);
}



void init()
{
	glViewport(0, 0, windowparam.width, windowparam.height);
	glMatrixMode(GL_PROJECTION_MATRIX);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);

	bool isOk = InitVSync();
	if (isOk) {
		SetVSyncState(enableVsync);
	}

	//glEnable(GL_ARB_explicit_attrib_location);

	//glFrontFace(GL_CCW);

	glClearColor(0.8, 0.8, 1, 1);



	initCubeVertex();
	initCubeMesh();
	initBuffers();

	if (loadColiisionModel)
	{
		//modelCollision.load("model\\bunny.obj");
		modelCollision.load("model\\triangle.obj");
		modelCollision.initGL_Buffer();
	}

	if (loadDefomModel)
	{
		deformModel.load("model\\bunny.obj");
		deformModel.normalize(0.5f,0.5f,0.5f,1.0f);
		deformModel.initGL_Buffer();
	}

	intiShaderProgram();

	if (UPDATE_METHOD==CL_GPU_UPDATE || UPDATE_METHOD==CL_CPU_UPDATE)
		UseCL::initCL();
}

Float3 springForce(const Float3 &p1, const Float3 &p2, const Float3 &v1, const Float3 &v2, float distance)
{

	Float3 force = { 0, 0, 0 };
	//f = -kx
	Float3 diff = (p1 - p2);
	Float3 diffV = (v1 - v2);
	//TODO 改成 epsilon
	if (diff.norm2() == 0.0f)
		return force;

	Float3 dir = diff / diff.norm2();
	float springForce = -(diff.norm2() - distance)*springForceK;

	if (springDampK != 0.0f)
	{
		float springDamping = diffV.dot(dir)*springDampK;
		springForce -= springDamping;
	}
	
	force = dir*(springForce);

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
bool testIntersectionModel = false;
void updatePosition(GLfloat *velosity, GLfloat *positionsSrc, GLfloat *positionsDst, bool enable_collision_detection)
{
	GLfloat* const vertex = positionsDst;
	GLfloat const GROUND = 0.0f;

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
			for (int j = 0; j<modelCollision.n_face; j+=1)
			{
				int v0Index = modelCollision.faces[j*3];
				int v1Index = modelCollision.faces[j*3 + 1];
				int v2Index = modelCollision.faces[j*3 + 2];

				Float3 v0{ modelCollision.vertex[v0Index * 4],
					modelCollision.vertex[v0Index * 4 + 1],
					modelCollision.vertex[v0Index * 4 + 2] };

				Float3 v1{ modelCollision.vertex[v1Index * 4],
					modelCollision.vertex[v1Index * 4 + 1],
					modelCollision.vertex[v1Index * 4 + 2] };

				Float3 v2{
					modelCollision.vertex[v2Index * 4],
					modelCollision.vertex[v2Index * 4 + 1],
					modelCollision.vertex[v2Index * 4 + 2]
				};

				Float3 diff = { vertex[i], vertex[i + 1], vertex[i + 2] };
				diff = diff - O;
				float t = 0.0f, u = 0.0f, v = 0.0f;


				if (intersect_triangle(O, diff, v0, v1, v2, &t, &u, &v) != 0)
				if (t <= 1.0f)
				{
					vertex[i + 1] = abs(vertex[i + 1]);
					//vertex[i+1] = 0.0f;//座標別改了 Q_Q
					velosity[i + 1] = BOUNCE_COEF*abs(velosity[i + 1]);//base on normal  內積normal之後 扣掉再加與normal同向?
					//velosity[i + 1] = 0.0f;

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
			//velosity[i] = 0.0f;
			//velosity[i + 2] = 0.0f;
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

void updateDeformModel()
{
	deformModel.cpuUpdateDeform(PATITION, cube_positions);
	deformModel.updateVertexBuffer();
}


void cpuUpdate()
{

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

	//testing function
	if (loadDefomModel)
	{
		updateDeformModel();
	}

	updateBufferObject();
}

void gameLoop()
{
	//for (int i = 0; i < cube_positions_size / 4; i += 4)
	//{
	//	printf("%f %f %f %f \n", cube_positions[i], cube_positions[i + 1], cube_positions[i + 2], cube_positions[i + 3]);
	//}
	if (enableUpdate)
	{

		switch (UPDATE_METHOD)
		{
			case NAIVE_CPU_UPDATE:
				cpuUpdate();
				break;
			case CL_GPU_UPDATE:
				UseCL::clUpdate();
				break;
			case CL_CPU_UPDATE:
				UseCL::clUpdate();
				updateBufferObject();
				break;
		}

		


	}
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


	switch (key)
	{
		case 27:
			exit(0);
			break;
		case '0':
			drawMode = 0;
			break;
		case '1':
			drawMode = 1;
			break;
		case '2':
			drawMode = 2;
			break;
		case '3':
			drawMode = 3;
			break;
		case '4':
			drawMode = 4;
			break;
		case 'P':
		case 'p':
			enableUpdate ^= 1;
			break;

		
		default:
			break;
	}


	return;
}




void UseCL::initCL()
{
	if (UPDATE_METHOD == CL_GPU_UPDATE)
		usePlatform = 1;
	else
	if (UPDATE_METHOD == CL_CPU_UPDATE)
	{
		usePlatform = 0;
	}

	global_size[0] = global_size[1] = global_size[2] = PATITION;
	//NULL means "I don't need this info"

	//Only get num of platforms
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
	std::cout << "num of platforms :" << num_platforms << endl;


	//Allocating yeah~~
	platforms = new cl_platform_id[num_platforms];
	//give all the platforms ID
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

	std::cout << "name of platforms : " << endl;
	bool interpo = false;
	for (int i = 0; i < num_platforms; i++)
	{
		std::cout << i + 1 << " ";
		cl_uint cl_platform_enum[] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION,
			CL_PLATFORM_EXTENSIONS, CL_PLATFORM_PROFILE };

		for (int j = 0; j < sizeof(cl_platform_enum) / sizeof(cl_uint); j++)
		{
			char *buffer;
			size_t num_char;
			clGetPlatformInfo(platforms[i], cl_platform_enum[j], 0, NULL, &num_char);
			buffer = new char[num_char + 1];
			clGetPlatformInfo(platforms[i], cl_platform_enum[j], num_char, buffer, &num_char);
			if (j>0)
				std::cout << ", ";
			std::cout << buffer;

			if (cl_platform_enum[j] == CL_PLATFORM_EXTENSIONS)
			{
				interpo = isCLGLInteropSupported(buffer);
			}

			delete[] buffer;
		}
		std::cout << endl;
		//TEST GL CL InteropSupported
		std::cout << "Suppert GL_CL interop Supperted?" << interpo << endl;


		std::cout << endl;
	}
	std::cout << endl;

	



	//get the device num
	if (UPDATE_METHOD == CL_GPU_UPDATE)
	{
		cout << "Use GPU devices" << endl;
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
		device_list = new cl_device_id[num_devices];

		//get the device list
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_GPU, num_devices, device_list, &num_devices);
	}
	else
	if (UPDATE_METHOD == CL_CPU_UPDATE)
	{
		cout << "Use CPU devices" << endl;
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_CPU, 0, NULL, &num_devices);
		device_list = new cl_device_id[num_devices];
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_CPU, num_devices, device_list, &num_devices);
	}

		


	std::cout << "device list" << endl;
	for (int i = 0; i < num_devices; i++)
	{
		std::cout << i + 1 << " ";
		char *buffer;
		size_t num_char;
		clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, 0, NULL, &num_char);
		buffer = new char[num_char + 1];
		clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, num_char, buffer, &num_char);
		std::cout << buffer << endl;
		delete[] buffer;
	}
	std::cout << endl;
	
	
	// 3. create a special opencl context based on the opengl context:
	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[usePlatform],
		0
	};
	
	if (UPDATE_METHOD == CL_GPU_UPDATE)
		context = clCreateContext(props, num_devices, device_list, NULL, NULL, &clStatus);
	else
	if (UPDATE_METHOD == CL_CPU_UPDATE)
		context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);
	cout << "create context" << clStatus << endl;
	//create a command queue
	command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);
	cout << "create command queue" << clStatus << endl;

	velosity_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);
	if (UPDATE_METHOD==CL_GPU_UPDATE)
		position_clmem = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, VBOs[0], &clStatus);
	else
		position_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);
	force_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);

	velosity_swap_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);
	position_swap_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);
	force_swap_clmem = clCreateBuffer(context, CL_MEM_READ_WRITE, cube_positions_size, NULL, &clStatus);

	//clear data
	clStatus = clEnqueueWriteBuffer(command_queue, velosity_clmem, CL_TRUE, 0, cube_positions_size, cube_velosity, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, force_clmem, CL_TRUE, 0, cube_positions_size, cube_force, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, position_clmem, CL_TRUE, 0, cube_positions_size, cube_positions, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, velosity_swap_clmem, CL_TRUE, 0, cube_positions_size, cube_velosity, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, force_swap_clmem, CL_TRUE, 0, cube_positions_size, cube_force, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, position_swap_clmem, CL_TRUE, 0, cube_positions_size, cube_positions, 0, NULL, NULL);

	clStatus = clFlush(command_queue);
	clStatus = clFinish(command_queue);
	

	char *kernelBuff=NULL;
	size_t code_size=0;
	kernelBuff = loadCLKernelFile(".\\cl_kernel\\updateForce.cl", &code_size);
	program_updateForce = clCreateProgramWithSource(context, 1, (const char**)&kernelBuff, NULL,&clStatus);

	clStatus = clBuildProgram(program_updateForce, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
	{
		cl_program program = program_updateForce;
		std::cout << "error" << endl;
		if (clStatus == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = new char[log_size];

			// Get the log
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
		}

		system("pause");
		exit(1);
	}

	kernel_updateForce = clCreateKernel(program_updateForce, "updateForce", &clStatus);	
	if (kernelBuff)
		delete[] kernelBuff;
	clStatus = clSetKernelArg(kernel_updateForce, 0, sizeof(frameT), &frameT);
	clStatus = clSetKernelArg(kernel_updateForce, 1, sizeof(cl_mem), &position_clmem);
	clStatus = clSetKernelArg(kernel_updateForce, 2, sizeof(cl_mem), &velosity_clmem);
	clStatus = clSetKernelArg(kernel_updateForce, 3, sizeof(cl_mem), &force_clmem);
	clStatus = clSetKernelArg(kernel_updateForce, 4, sizeof(Gravity), &Gravity);
	clStatus = clSetKernelArg(kernel_updateForce, 5, sizeof(patition_dis), &patition_dis);
	clStatus = clSetKernelArg(kernel_updateForce, 6, sizeof(springForceK), &springForceK);
	clStatus = clSetKernelArg(kernel_updateForce, 7, sizeof(springDampK), &springDampK);

	
	code_size = 0;
	kernelBuff = loadCLKernelFile(".\\cl_kernel\\applyForce.cl", &code_size);
	program_applyForce = clCreateProgramWithSource(context, 1, (const char **)&kernelBuff, NULL, &clStatus);
	clStatus = clBuildProgram(program_applyForce, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
	{
		cl_program program = program_applyForce;
		std::cout << "error" << endl;
		if (clStatus == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log
			char *log = new char[log_size];

			// Get the log
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
		}

		system("pause");
		exit(1);
	}
	delete[] kernelBuff;

	kernel_applyForce = clCreateKernel(program_applyForce, "applyForce", &clStatus);
		
	clStatus = clSetKernelArg(kernel_applyForce, 0, sizeof(frameT), &frameT);
	clStatus = clSetKernelArg(kernel_applyForce, 1, sizeof(cl_mem), &force_clmem);
	clStatus = clSetKernelArg(kernel_applyForce, 2, sizeof(cl_mem), &velosity_clmem);
	clStatus = clSetKernelArg(kernel_applyForce, 3, sizeof(cl_mem), &velosity_clmem);
	


	code_size = 0;
	kernelBuff = loadCLKernelFile(".\\cl_kernel\\updatePosition.cl", &code_size);
	program_updatePosition = clCreateProgramWithSource(context, 1, (const char**)&kernelBuff, NULL, &clStatus);
	clStatus = clBuildProgram(program_updatePosition, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
	{
		cl_program program = program_updatePosition;
		std::cout << "error" << endl;
		if (clStatus == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log

			char *log = new char[log_size];

			// Get the log
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
		}

		system("pause");
		exit(1);
	}

	delete[]kernelBuff;

	kernel_updatePosition = clCreateKernel(program_updatePosition, "updatePosition", &clStatus);

	int enable_collision = true;
	clStatus = clSetKernelArg(kernel_updatePosition, 0, sizeof(frameT), &frameT);
	clStatus = clSetKernelArg(kernel_updatePosition, 1, sizeof(cl_mem), &velosity_clmem);
	clStatus = clSetKernelArg(kernel_updatePosition, 2, sizeof(cl_mem), &position_clmem);
	clStatus = clSetKernelArg(kernel_updatePosition, 3, sizeof(cl_mem), &position_clmem);
	clStatus = clSetKernelArg(kernel_updatePosition, 4, sizeof(int), &enable_collision);


	kernelBuff = loadCLKernelFile(".\\cl_kernel\\applyForceIEM.cl", &code_size);
	program_applyForceIEM = clCreateProgramWithSource(context, 1, (const char**)&kernelBuff, NULL, &clStatus);
	clStatus = clBuildProgram(program_applyForceIEM, 1, device_list, NULL, NULL, NULL);
	if (clStatus != CL_SUCCESS)
	{
		cl_program program = program_applyForceIEM;
		std::cout << "error" << endl;
		if (clStatus == CL_BUILD_PROGRAM_FAILURE) {
			// Determine the size of the log
			size_t log_size;
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			// Allocate memory for the log

			char *log = new char[log_size];

			// Get the log
			clGetProgramBuildInfo(program, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

			// Print the log
			printf("%s\n", log);
			delete[] log;
		}

		system("pause");
		exit(1);
	}
	delete[] kernelBuff;
	kernel_applyForceIEM = clCreateKernel(program_applyForceIEM, "applyForceIEM", &clStatus);
	clStatus = clSetKernelArg(kernel_applyForceIEM, 0, sizeof(frameT), &frameT);
	clStatus = clSetKernelArg(kernel_applyForceIEM, 1, sizeof(force_clmem), &force_clmem);
	clStatus = clSetKernelArg(kernel_applyForceIEM, 2, sizeof(force_swap_clmem), &force_swap_clmem);
	clStatus = clSetKernelArg(kernel_applyForceIEM, 3, sizeof(velosity_clmem), &velosity_clmem);
}



void UseCL::clUpdate()
{
	cl_int clStatus;

	clEnqueueAcquireGLObjects(command_queue, 1, &position_clmem, 0, NULL, NULL);

	if (INTEGRATION == EULAR_METHOD)
	{
		clStatus = clSetKernelArg(kernel_updateForce, 1, sizeof(cl_mem), &position_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 2, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 3, sizeof(cl_mem), &force_clmem);
		clStatus = clSetKernelArg(kernel_applyForce, 1, sizeof(cl_mem), &force_clmem);
		clStatus = clSetKernelArg(kernel_applyForce, 2, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_applyForce, 3, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 1, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 2, sizeof(cl_mem), &position_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 3, sizeof(cl_mem), &position_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updateForce, 3, NULL, global_size, local_size, 0, NULL, NULL);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_applyForce, 3, NULL, global_size, local_size, 0, NULL, NULL);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updatePosition, 3, NULL, global_size, local_size, 0, NULL, NULL);

		if (UPDATE_METHOD == CL_CPU_UPDATE)
		{
			clStatus = clEnqueueReadBuffer(command_queue, position_clmem, CL_TRUE, 0, cube_positions_size, cube_positions, 0, NULL, NULL);
		}

		clStatus = clFlush(command_queue);
		clStatus = clFinish(command_queue);
	}
	else
	if (INTEGRATION == IMPROVED_EULAR_METHOD)
	{
		//calc u(t+h)
		clStatus = clSetKernelArg(kernel_updateForce, 1, sizeof(cl_mem), &position_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 2, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 3, sizeof(cl_mem), &force_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updateForce, 3, NULL, global_size, local_size, 0, NULL, NULL);

		clStatus = clSetKernelArg(kernel_applyForce, 1, sizeof(cl_mem), &force_clmem);
		clStatus = clSetKernelArg(kernel_applyForce, 2, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_applyForce, 3, sizeof(cl_mem), &velosity_swap_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_applyForce, 3, NULL, global_size, local_size, 0, NULL, NULL);

		clStatus = clSetKernelArg(kernel_updatePosition, 1, sizeof(cl_mem), &velosity_swap_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 2, sizeof(cl_mem), &position_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 3, sizeof(cl_mem), &position_swap_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updatePosition, 3, NULL, global_size, local_size, 0, NULL, NULL);

		//calc g(u(t+h),t+h)
		clStatus = clSetKernelArg(kernel_updateForce, 1, sizeof(cl_mem), &position_swap_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 2, sizeof(cl_mem), &velosity_swap_clmem);
		clStatus = clSetKernelArg(kernel_updateForce, 3, sizeof(cl_mem), &force_swap_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updateForce, 3, NULL, global_size, local_size, 0, NULL, NULL);

		//TODO implement applyForceIEM
		clStatus = clSetKernelArg(kernel_applyForceIEM, 1, sizeof(force_clmem), &force_clmem);
		clStatus = clSetKernelArg(kernel_applyForceIEM, 2, sizeof(force_swap_clmem), &force_swap_clmem);
		clStatus = clSetKernelArg(kernel_applyForceIEM, 3, sizeof(velosity_clmem), &velosity_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_applyForceIEM, 3, NULL, global_size, local_size, 0, NULL, NULL);


		clStatus = clSetKernelArg(kernel_updatePosition, 1, sizeof(cl_mem), &velosity_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 2, sizeof(cl_mem), &position_clmem);
		clStatus = clSetKernelArg(kernel_updatePosition, 3, sizeof(cl_mem), &position_clmem);
		clStatus = clEnqueueNDRangeKernel(command_queue, kernel_updatePosition, 3, NULL, global_size, local_size, 0, NULL, NULL);

		if (UPDATE_METHOD == CL_CPU_UPDATE)
		{
			clStatus = clEnqueueReadBuffer(command_queue, position_clmem, CL_TRUE, 0, cube_positions_size, cube_positions, 0, NULL, NULL);
		}
		clStatus = clFlush(command_queue);
		clStatus = clFinish(command_queue);
	}

	clEnqueueReleaseGLObjects(command_queue, 1, &position_clmem, 0, NULL, NULL);


	return;
}

bool UseCL::isCLGLInteropSupported(char* extensionString)
{
	std::string allStrings(extensionString);
	std::string searchString("cl_khr_gl_sharing");
	std::size_t index = allStrings.find(searchString);
	if (std::string::npos == index)
	{
		return false;
	}
	else
	{
		return true;
	}
}

char *loadCLKernelFile(char* fileName, size_t *retStringSize)
{
	FILE *fin = NULL;
	fopen_s(&fin, fileName, "rb");

	if (fin == NULL)
	{
		std::cout << "can not found file" << fileName << endl;
		system("pause");
		exit(1);
	}

	fseek(fin, 0, SEEK_END);
	*retStringSize = ftell(fin);
	fseek(fin, 0, SEEK_SET);


	char *data = NULL;
	data = new char[*retStringSize + 1];

	if (fread(data, *retStringSize, 1, fin) != 1)
	{
		fclose(fin);
		delete[] data;
		return 0;
	}

	data[*retStringSize] = '\0';
	fclose(fin);
	return data;
}

