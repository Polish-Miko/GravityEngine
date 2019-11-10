#pragma once
#include "GGiPreInclude.h"


class GGiFloat3
{
public:
	GGiFloat3();

	GGiFloat3(float xi,float yi, float zi) : x(xi), y(yi), z(zi) {}

	~GGiFloat3();

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	inline float& operator[](int i)
	{
		switch (i)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			return x;
		}
	}

	inline GGiFloat3 operator-(const GGiFloat3& i)
	{
		GGiFloat3 o;
		o.x = x - i.x;
		o.y = y - i.y;
		o.z = z - i.z;
		return o;
	}

	inline GGiFloat3 operator+(const GGiFloat3& i)
	{
		GGiFloat3 o;
		o.x = x + i.x;
		o.y = y + i.y;
		o.z = z + i.z;
		return o;
	}

	inline GGiFloat3 operator*(const GGiFloat3& i)
	{
		GGiFloat3 o;
		o.x = x * i.x;
		o.y = y * i.y;
		o.z = z * i.z;
		return o;
	}

	inline GGiFloat3 operator*(const float& i)
	{
		GGiFloat3 o;
		o.x = x * i;
		o.y = y * i;
		o.z = z * i;
		return o;
	}

	static inline GGiFloat3 Cross(GGiFloat3& i1, GGiFloat3& i2)
	{
		GGiFloat3 o;
		o.x = i1.y * i2.z - i2.y * i1.z;
		o.y = i1.z * i2.x - i1.x * i2.z;
		o.z = i1.x * i2.y - i2.x * i1.y;
		return o;
	}

	static inline float Dot(GGiFloat3& i1, GGiFloat3& i2)
	{
		return (i1.x * i2.x + i1.y * i2.y + i1.z * i2.z);
	}

	static inline GGiFloat3 Normalize(GGiFloat3 i)
	{
		float len = sqrt(i.x * i.x + i.y * i.y + i.z * i.z);
		GGiFloat3 o = i;
		o = o * (1 / len);
		return o;
	}
};

