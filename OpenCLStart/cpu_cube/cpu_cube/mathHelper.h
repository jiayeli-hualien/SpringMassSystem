#ifndef JIAYELI_MATH_HELPER
#define JIAYELI_MATH_HELPER
#include<math.h>
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
Float3 operator*(const float m, const Float3 &f3);
Float3 cross(const Float3 &v1, const Float3 &v2);

int intersect_triangle(const Float3 &orig, const Float3 &dir,
	const Float3 &vert0, const Float3 &vert1, const Float3 &vert2,
	float *t, float *u, float *v);


#endif