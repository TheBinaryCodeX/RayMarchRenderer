#pragma once
#include <iostream>
#include <math.h>
namespace Vector
{
	class Vector2
	{
	public:
		double x;
		double y;

		// Add

		inline Vector2 operator+(Vector2 vector)
		{
			return Vector2(x + vector.x, y + vector.y);
		}

		inline void operator+=(Vector2 vector)
		{
			x += vector.x;
			y += vector.y;
		}

		// Subtract

		inline Vector2 operator-(Vector2 vector)
		{
			return Vector2(x - vector.x, y - vector.y);
		}

		inline void operator-=(Vector2 vector)
		{
			x -= vector.x;
			y -= vector.y;
		}

		// Multiply

		inline Vector2 operator*(double value)
		{
			return Vector2(x * value, y * value);
		}

		inline void operator*=(double value)
		{
			x *= value;
			y *= value;
		}

		inline Vector2 operator*(Vector2 vector)
		{
			return Vector2(x * vector.x, y * vector.y);
		}

		inline void operator*=(Vector2 vector)
		{
			x *= vector.x;
			y *= vector.y;
		}

		// Divide

		inline Vector2 operator/(double value)
		{
			return Vector2(x / value, y / value);
		}

		inline void operator/=(double value)
		{
			x /= value;
			y /= value;
		}

		inline Vector2 operator/(Vector2 vector)
		{
			return Vector2(x / vector.x, y / vector.y);
		}

		inline void operator/=(Vector2 vector)
		{
			x /= vector.x;
			y /= vector.y;
		}

		// Compare

		inline bool operator==(Vector2 vector)
		{
			if (x == vector.x && y == vector.y)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		inline bool operator!=(Vector2 vector)
		{
			if (x != vector.x || y != vector.y)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		// Access
		inline double& operator[](int i)
		{
			if (i == 0)
			{
				return x;
			}
			else if (i == 1)
			{
				return y;
			}
			else
			{
				std::cerr << i << " is out of Vector bounds" << std::endl;
			}
		}
		// End of Operators

		Vector2()
		{
			x = 0;
			y = 0;
		}

		Vector2(double xPos, double yPos)
		{
			x = xPos;
			y = yPos;
		}

		~Vector2()
		{

		}

		inline double magnitude()
		{
			return sqrt(x*x + y*y);
		}

		inline double dot(Vector2 vector)
		{
			return x * vector.x + y * vector.y;
		}

		inline Vector2 normalized()
		{
			return *this / magnitude();
		}
	};

	class Vector3
	{
	public:
		double x;
		double y;
		double z;

		// Add

		inline Vector3 operator+(Vector3 vector)
		{
			return Vector3(x + vector.x, y + vector.y, z + vector.z);
		}

		inline void operator+=(Vector3 vector)
		{
			x += vector.x;
			y += vector.y;
			z += vector.z;
		}

		// Subtract

		inline Vector3 operator-(Vector3 vector)
		{
			return Vector3(x - vector.x, y - vector.y, z - vector.z);
		}

		inline void operator-=(Vector3 vector)
		{
			x -= vector.x;
			y -= vector.y;
			z -= vector.z;
		}

		// Multiply

		inline Vector3 operator*(double value)
		{
			return Vector3(x * value, y * value, z * value);
		}

		inline void operator*=(double value)
		{
			x *= value;
			y *= value;
			z *= value;
		}

		inline Vector3 operator*(Vector3 vector)
		{
			return Vector3(x * vector.x, y * vector.y, z * vector.z);
		}

		inline void operator*=(Vector3 vector)
		{
			x *= vector.x;
			y *= vector.y;
			z *= vector.z;
		}

		// Divide

		inline Vector3 operator/(double value)
		{
			return Vector3(x / value, y / value, z / value);
		}

		inline void operator/=(double value)
		{
			x /= value;
			y /= value;
			z /= value;
		}

		inline Vector3 operator/(Vector3 vector)
		{
			return Vector3(x / vector.x, y / vector.y, z / vector.z);
		}

		inline void operator/=(Vector3 vector)
		{
			x /= vector.x;
			y /= vector.y;
			z /= vector.z;
		}

		// Compare

		inline bool operator==(Vector3 vector)
		{
			if (x == vector.x && y == vector.y && z == vector.z)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		inline bool operator!=(Vector3 vector)
		{
			if (x != vector.x || y != vector.y || z != vector.z)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		// Access
		inline double& operator[](int i)
		{
			if (i == 0)
			{
				return x;
			}
			else if (i == 1)
			{
				return y;
			}
			else if (i == 2)
			{
				return z;
			}
			else
			{
				std::cerr << i << " is out of Vector bounds" << std::endl;
			}
		}
		// End of Operators

		Vector3()
		{
			x = 0;
			y = 0;
			z = 0;
		}

		Vector3(double xPos, double yPos, double zPos)
		{
			x = xPos;
			y = yPos;
			z = zPos;
		}

		~Vector3()
		{

		}

		inline double magnitude()
		{
			return sqrt(x*x + y*y + z*z);
		}

		inline double dot(Vector3 vector)
		{
			return x * vector.x + y * vector.y + z * vector.z;
		}

		inline Vector3 cross(Vector3 vector)
		{
			return Vector3(y * vector.z - z * vector.y, z * vector.x - x * vector.z, x * vector.y - y * vector.x);
		}

		inline Vector3 normalized()
		{
			return magnitude() <= 0 ? *this : *this / magnitude();
		}
	};

	// Print

	inline std::ostream& operator<<(std::ostream& os, const Vector2& vector)
	{
		os << "(" << vector.x << ", " << vector.y << ")";
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const Vector3& vector)
	{
		os << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
		return os;
	}
}

