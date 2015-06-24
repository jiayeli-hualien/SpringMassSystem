#include "LoadObj.h"


LoadObj::LoadObj()
{
	this->faces = NULL;
	this->normal = NULL;
	this->vertex = NULL;
	this->n_face = 0;
	this->n_normal = 0;
	this->n_vertex = 0;
}


LoadObj::~LoadObj()
{
	if (this->faces)
		delete [] this->faces;
	if (this->normal)
		delete [] this->normal;
	if (this->vertex)
		delete [] this->vertex;
}


int LoadObj::load(const string& fileName)
{
	ifstream fin(fileName);

	string line;
	while (getline(fin, line))
	{
		//cout << line << endl;
		istringstream sin(line);
		string type;
		sin >> type;
		if (type == string("v"))
			this->n_vertex++;
		else
		if (type == string("vn"))
			this->n_normal++;
		else
		if (type == string("f"))
			this->n_face++;
	}

	this->vertex = new GLfloat[4*this->n_vertex];
	if (n_normal>0)
	this->normal = new GLfloat[4*this->n_normal];
	this->faces = new GLuint[3*this->n_face];

	fin.close();
	fin.open(fileName);

	GLsizei indexV = 0;
	GLsizei indexN = 0;
	GLsizei indexF = 0;
	while (getline(fin, line))
	{
		istringstream sin(line);
		string type;
		sin >> type;
		if (type == string("v"))
		{
			sin >> this->vertex[indexV];
			sin >> this->vertex[indexV+1];
			sin >> this->vertex[indexV+2];
			this->vertex[indexV+3] = 1.0f;
			indexV += 4;
		}
		else
		if (type == string("vn"))
		{
			sin >> this->normal[indexN];
			sin >> this->normal[indexN + 1];
			sin >> this->normal[indexN + 2];
			this->normal[indexN + 3] = 1.0f;
			indexN += 4;
		}
		else
		if (type == string("f"))
		{
			char eat1, eat2;
			GLint temp1, temp2, temp3;
			sin >> this->faces[indexF] >> eat1 >> eat2>> temp1;
			sin >> this->faces[indexF + 1] >> eat1 >> eat2 >> temp2;
			sin >> this->faces[indexF + 2] >> eat1 >> eat2 >> temp3;

			this->faces[indexF]--;
			this->faces[indexF + 1]--;
			this->faces[indexF + 2]--;
			//cout << this->_faces[indexF] << " " << this->_faces[indexF+1] << " " << this->_faces[indexF+2] << endl;

			indexF += 3;
		}
	}


	return 1;
}

void LoadObj::initGL_Buffer()
{
	glGenBuffers(1,&(this->ebo));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*this->n_face * 3, this->faces, GL_STATIC_DRAW);

	glGenBuffers(1,&(this->vao));
	glBindBuffer(GL_VERTEX_ARRAY, this->vao);


	glGenBuffers(1, &(this->vbo_vertex));
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertex);


	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*(this->n_vertex * 4 + this->n_normal * 4), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*(this->n_vertex*4), this->vertex);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat)*(this->n_vertex*4), sizeof(GLfloat)*(this->n_normal*4), this->normal);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid * const) (sizeof(GLfloat)*(this->n_vertex)*4));//offset

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void LoadObj::draw()
{

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBindBuffer(GL_VERTEX_ARRAY, vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex);


	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);//offset start from 0
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid * const)(sizeof(GLfloat)*(this->n_vertex)*4));//offset

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);


	glDrawElements(GL_TRIANGLES, this->n_face*3, GL_UNSIGNED_INT,0);
	//glDrawArrays(GL_POINTS, 0, this->n_vertex);
}
