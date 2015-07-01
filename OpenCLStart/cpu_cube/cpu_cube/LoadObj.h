#pragma once
#include<GL/glew.h>
#include<GL/freeglut.h>
#include<iostream>
#include<string>
#include<fstream>
#include<sstream>
#include"mathHelper.h"
#include<glm/gtc/matrix_transform.hpp>
using namespace std;
class LoadObj
{
public:
	GLfloat *vertex;//xyzw
	GLfloat *normal;
	GLuint *faces;//OpenGL element
	GLsizei n_vertex;
	GLsizei n_normal;
	GLsizei n_face;//number of faces

	GLuint vao;
	GLuint vbo_vertex;
	GLuint ebo;

public:
	LoadObj();
	~LoadObj();

	int load(const string &fileName);

	void initGL_Buffer();
	void draw();
	void updateNormal();
	void normalize(float tcx, float tcy, float tcz, float tsize);
};

class DeformObj : public LoadObj{
private:
	GLfloat *back_up_vertex;
public:
	int load(const string &fileName);
	void cpuUpdateDeform(const int patition, GLfloat *cube_vertex);
	void updateVertexBuffer();
	void normalize(float tcx, float tcy, float tcz, float tsize);
	~DeformObj();
};
