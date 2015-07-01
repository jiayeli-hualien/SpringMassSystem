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
			string token1, token2, token3;
			sin >> token1 >> token2 >> token3;

			istringstream paser;
			paser.str(token1);


			char eat1, eat2;
			GLint temp1, temp2, temp3;
			paser >> this->faces[indexF] >> eat1 >> eat2>> temp1;
			paser.clear();
			paser.str(token2);
			paser >> this->faces[indexF + 1] >> eat1 >> eat2 >> temp2;
			paser.clear();
			paser.str(token3);
			paser >> this->faces[indexF + 2] >> eat1 >> eat2 >> temp3;

			this->faces[indexF]--;
			this->faces[indexF + 1]--;
			this->faces[indexF + 2]--;
			//cout << this->_faces[indexF] << " " << this->_faces[indexF+1] << " " << this->_faces[indexF+2] << endl;

			indexF += 3;
		}
	}


	//complete normal
	if (this->n_normal == 0)
	{
		this->n_normal = this->n_vertex;

		this->normal = new GLfloat[4 * this->n_normal];

		updateNormal();


	}

	//this->normalize(0.5f, 0.5f, 0.5f, 1.0f);

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


	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*(this->n_vertex * 4 + this->n_normal * 4), NULL, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*(this->n_vertex*4), this->vertex);

	if (this->normal)
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

void LoadObj::updateNormal()
{
	//use w as counting
	const int range = this->n_normal * 4;
	for (int i = 0; i < range; i += 4)
	{
		this->normal[i] = 0.0f;
		this->normal[i + 1] = 0.0f;
		this->normal[i + 2] = 0.0f;
		this->normal[i + 3] = 0.0f;
	}

	const int rangeF = this->n_face * 3;
	for (int i = 0; i < rangeF; i+=3)
	{
		int v1 = faces[i];
		int v2 = faces[i+1];
		int v3 = faces[i+2];

		Float3 V1 = { this->vertex[v1 * 4], this->vertex[v1 * 4 + 1], this->vertex[v1 * 4 + 2] };
		Float3 V2 = { this->vertex[v2 * 4], this->vertex[v2 * 4 + 1], this->vertex[v2 * 4 + 2] };
		Float3 V3 = { this->vertex[v3 * 4], this->vertex[v3 * 4 + 1], this->vertex[v3 * 4 + 2] };

		Float3 n = cross(V2 - V1, V3 - V1);
		n = n/n.norm2();
		
		this->normal[v1 * 4]+=n.x;
		this->normal[v1 * 4+1]+=n.y;
		this->normal[v1 * 4+2]+=n.z;
		this->normal[v1 * 4+3]+=1.0f;
		this->normal[v2 * 4] += n.x;
		this->normal[v2 * 4 + 1] += n.y;
		this->normal[v2 * 4 + 2] += n.z;
		this->normal[v2 * 4 + 3] += 1.0f;
		this->normal[v3 * 4] += n.x;
		this->normal[v3 * 4 + 1] += n.y;
		this->normal[v3 * 4 + 2] += n.z;
		this->normal[v3 * 4 + 3] += 1.0f;
		
	}

	for (int i = 0; i < range; i += 4)
	{
		if (this->normal[i + 3] != 0.0f)
		{
			this->normal[i] /= this->normal[i + 3];
			this->normal[i + 1] /= this->normal[i + 3];
			this->normal[i + 2] /= this->normal[i + 3];
		}
			
		this->normal[i + 3] = 1.0f;
	}
}



void LoadObj::normalize(float tcx, float tcy, float tcz, float tsize)
{
	//目前重心
	const int vertexR = this->n_vertex * 4;
	float center[3] = {0.0f,0.0f,0.0f};
	float upper[3] = {vertex[0],vertex[1],vertex[2]};
	float lower[3] = {vertex[0], vertex[1], vertex[2]};
	int counting = 0;
	for (int i = 0; i < vertexR; i += 4)
	{
		for (int j = 0; j < 3; j++)
		{
			if (vertex[i + j]>upper[j])
				upper[j] = vertex[i + j];
			if (vertex[i + j]<lower[j])
				lower[j] = vertex[i + j];
		}


	}
	center[0] = (upper[0] + lower[0])*0.5f;
	center[1] = (upper[1] + lower[1])*0.5f;
	center[2] = (upper[2] + lower[2])*0.5f;

	float scale = upper[0] - lower[1];
	for (int i = 1; i < 3; i++)
	{
		if( (upper[i] - lower[i]) > scale)
			scale = upper[i] - lower[i];
	}
	scale = tsize * 0.99f / scale;
	glm::mat4x4 modelTransform = glm::mat4(1);
	modelTransform = glm::translate(modelTransform, glm::vec3(tcx, tcy, tcz));
	modelTransform = glm::scale(modelTransform, glm::vec3(scale, scale, scale));
	modelTransform = glm::translate(modelTransform, glm::vec3(-center[0], -center[1], -center[2]));


	for (int i = 0; i < vertexR; i += 4)
	{
		glm::vec4 position(vertex[i], vertex[i + 1], vertex[i + 2], vertex[i + 3]);

		position = modelTransform*position;

		vertex[i] = position.x / position.w;
		vertex[i + 1] = position.y / position.w;
		vertex[i + 2] = position.z / position.w;
		vertex[i + 3] = 1.0f;

		if (vertex[i] >= 1.0f || vertex[i+1]>=1.0f || vertex[i+2]>=1.0f)
			printf("OMG %f %f %f\n", vertex[i], vertex[i+1], vertex[i+2]);
		if (vertex[i] < 0.0f || vertex[i + 1] < 0.0f || vertex[i + 2] < 0.0f)
			printf("OMG %f %f %f\n", vertex[i], vertex[i + 1], vertex[i + 2]);
	}

}

int DeformObj::load(const string &fileName){
	int retV = LoadObj::load(fileName);
	if (retV != 1)
		return retV;
	back_up_vertex = NULL;
	back_up_vertex = new GLfloat[4 * this->n_vertex];

	memcpy(back_up_vertex, vertex, sizeof(GLfloat)*4*this->n_vertex);

	return 1;
}

void DeformObj::normalize(float tcx, float tcy, float tcz, float tsize)
{
	LoadObj::normalize(tcx, tcy, tcz, tsize);
	if (back_up_vertex)
		memcpy(back_up_vertex, vertex, sizeof(GLfloat)* 4 * this->n_vertex);
}

DeformObj::~DeformObj()
{
	if (back_up_vertex)
		delete[] back_up_vertex;
}

void DeformObj::updateVertexBuffer()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBindBuffer(GL_VERTEX_ARRAY, vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertex);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*(this->n_vertex * 4), this->vertex);
}

int clampInt(int L, int U, int v)
{
	if (v > U)
		return U;
	if (v < L)
		return L;
	return v;
}

inline int GET_INDEX(const int x, const int y, const int z, const int PATITION)
{
	return (PATITION*PATITION*(x)+PATITION*(y)+z) * 4;
}
void DeformObj::cpuUpdateDeform(const int patition, GLfloat *cube_vertex)
{
	const float invPatition = 1.0f / (float)(patition-1);
	const int rangeV = this->n_vertex * 4;
	for (int i = 0; i < rangeV; i += 4)
	{
		//read backup
		Float3 V = {back_up_vertex[i], back_up_vertex[i+1], back_up_vertex[i+2]};
		float s = (V.x);
		int cube_index_x = (int)floor(s*(patition - 1));
		cube_index_x = clampInt(0, patition-1, cube_index_x);
		s = (s - cube_index_x*invPatition)*(patition-1);

		float t = V.y;
		int cube_index_y = (int)floor(t*(patition - 1));
		cube_index_y = clampInt(0, patition-1, cube_index_y);

		t = (t - cube_index_y*invPatition)*(patition-1);


		float u = V.z;
		int cube_index_z = (int)floor(u*(patition - 1));
		cube_index_z = clampInt(0, patition-1, cube_index_z);

		u = (u - cube_index_z*invPatition)*(patition-1);

		//check
		//if (cube_index_x >= patition || cube_index_x < 0 || cube_index_y >= patition || cube_index_y < 0 || cube_index_z >= patition || cube_index_z < 0)
			//printf("update OMG %d %d %d", cube_index_x, cube_index_y, cube_index_z);
		//if (s > 1.0f || s<0.0f || t>1.0f || t<0.0f || u>1.0f || u < 0.0f)
		//{
//			printf("update OMG2 %f %f %f\n",s,t,u);
	//	}

		//interpolation

		//https://en.wikipedia.org/wiki/Trilinear_interpolation
		const int index000 = GET_INDEX(cube_index_x, cube_index_y, cube_index_z, patition);
		const Float3 V000 = {cube_vertex[index000], cube_vertex[index000+1], cube_vertex[index000+2]};

		const int index001 = GET_INDEX(cube_index_x, cube_index_y, cube_index_z+1, patition);
		const Float3 V001 = { cube_vertex[index001], cube_vertex[index001 + 1], cube_vertex[index001 + 2] };

		const int index010 = GET_INDEX(cube_index_x, cube_index_y+1, cube_index_z, patition);
		const Float3 V010 = { cube_vertex[index010], cube_vertex[index010 + 1], cube_vertex[index010 + 2] };

		const int index011 = GET_INDEX(cube_index_x, cube_index_y+1, cube_index_z+1, patition);
		const Float3 V011 = { cube_vertex[index011], cube_vertex[index011 + 1], cube_vertex[index011 + 2] };

		const int index100 = GET_INDEX(cube_index_x+1, cube_index_y, cube_index_z, patition);
		const Float3 V100 = { cube_vertex[index100], cube_vertex[index100 + 1], cube_vertex[index100 + 2] };

		const int index101 = GET_INDEX(cube_index_x+1, cube_index_y, cube_index_z+1, patition);
		const Float3 V101 = { cube_vertex[index101], cube_vertex[index101 + 1], cube_vertex[index101 + 2] };

		const int index110 = GET_INDEX(cube_index_x+1, cube_index_y+1, cube_index_z, patition);
		const Float3 V110 = { cube_vertex[index110], cube_vertex[index110 + 1], cube_vertex[index110 + 2] };

		const int index111 = GET_INDEX(cube_index_x+1, cube_index_y+1, cube_index_z+1, patition);
		const Float3 V111 = { cube_vertex[index111], cube_vertex[index111 + 1], cube_vertex[index111 + 2] };

		const Float3 V00 = V000*(1.0f-s)+V100*(s);
		const Float3 V01 = V001*(1.0f - s) + V101*(s);
		const Float3 V10 = V010*(1.0f - s) + V110*(s);
		const Float3 V11 = V011*(1.0f - s) + V111*(s);

		const Float3 V0 = V00*(1.0f - t) + V10*t;
		const Float3 V1 = V01*(1.0f - t) + V11*t;
		
		//write vertex
		const Float3 ans = V0*(1.0f - u) + V1*u;

		this->vertex[i] = ans.x;
		this->vertex[i+1] = ans.y;
		this->vertex[i + 2] = ans.z;

		
	}
}