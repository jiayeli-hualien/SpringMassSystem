#ifndef JIAYELI_MATH_HELPER
#define JIAYELI_MATH_HELPER
struct Float3
{
	float x, y, z;

	Float3 operator- (const Float3 &other) const
	{
		return { this->x - other.x, this->y - other.y, this->z - other.z};
	}
	Float3 operator*(const float &m) const
	{
		return { this->x*m, this->y*m, this->z*m};
	}
	Float3 operator/(const float &d) const
	{
		return { this->x / d, this->y / d, this->z / d };
	}

	float dot(const Float3 &other) const
	{
		return this->x*other.x + this->y*other.y + this->z*other.z;
	}

	const Float3& operator+=(const Float3 &f2)
	{
		this->x += f2.x;
		this->y += f2.y;
		this->z += f2.z;
		return *this;
	}

	Float3 operator+(const Float3 &p2) const
	{
		return { this->x + p2.x, this->y + p2.y, this->z + p2.z };
	}

	float norm2()
	{
		return sqrt(x*x + y*y + z*z);
	}

	Float3 operator-()
	{
		return { -this->x, -this->y, -this->z };
	}


};
Float3 operator*(const float m, const Float3 &f3)
{
	Float3 ans = { m*f3.x, m*f3.y, m*f3.z };
	return ans;
}
Float3 cross(const Float3 &v1, const Float3 &v2)
{
	return{ v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x };
}

#define TEST_CULL
//MT97
//ray triangle intersection
const float INTEC_EPSILON = 0.000001f;
int intersect_triangle(const Float3 &orig, const Float3 &dir,  
					   const Float3 &vert0, const Float3 &vert1, const Float3 &vert2,
						float *t, float *u, float *v)
{
	Float3 edge1, edge2, tvec, pvec, qvec;
	float det, inv_det;

	edge1 = vert1 - vert0;
	edge2 = vert2 - vert0;

	pvec = cross(dir,edge2);
	det = edge1.dot(pvec);

#ifdef TEST_CULL
	if (det<INTEC_EPSILON)
		return 0;
	tvec = orig - vert0;
	*u = tvec.dot(pvec);
	if (*u<0.0f || *u>det)//>1
		return 0;
	qvec = cross(tvec, edge1);
	*v = dir.dot(qvec);
	if (*v<0.0 || *u + *v > det)
		return 0;
	*t = edge2.dot(qvec);
	inv_det = 1.0f/det;
	*t *= inv_det;
	*u *= inv_det;
	*v *= inv_det;
#else//non culling
	if (det > -INTEC_EPSILON && det < INTEC_EPSILON)
		return 0;//near paralel

	inv_det = 1.0f / det;

	tvec = orig - vert0;
	*u = tvec.dot(pvec) * inv_det;
	if (*u<0.0f || *u>1.0f)
		return 0;

	qvec = cross(tvec, edge1);

	*v = dir.dot(qvec) * inv_det;

	if (*v < 0.0f || *u + *v >1.0f)
		return 0;

	*t = edge2.dot(qvec) * inv_det;

#endif
	return 1;
}


#endif