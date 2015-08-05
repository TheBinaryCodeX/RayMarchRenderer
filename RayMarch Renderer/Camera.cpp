#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
{

}

Camera::Camera(Vector3 eyePos, Vector3 lookDir, float aspect, float fov)
{
	this->aspect = aspect;
	this->fov = fov;

	eye = eyePos;
	dir = lookDir;

	calculateRays();
}

Camera::~Camera()
{

}

void getLocal(Vector3& x, Vector3& y, Vector3 z)
{
	x = z.cross(Vector3(0, 1, 0));
	y = z.cross(x);
}

Vector3 rotAxis(Vector3 u, float t, Vector3 point, Vector3 origin)
{
	glm::mat3 uc = glm::mat3
		(
		0, -u.z, u.y,
		u.z, 0, -u.x,
		-u.y, u.x, 0
		);

	glm::mat3 ut = glm::mat3
		(
		u.x * u.x, u.x * u.y, u.x * u.z,
		u.x * u.y, u.y * u.y, u.y * u.z,
		u.x * u.z, u.y * u.z, u.z * u.z
		);

	glm::mat3 R = cos(t) * glm::mat3(1) + sin(t) * uc + (1 - cos(t)) * ut;

	glm::vec3 P = R * glm::vec3(point.x - origin.x, point.y - origin.y, point.z - origin.z);

	return Vector3(P.x + origin.x, P.y + origin.y, P.z + origin.z);
}

void Camera::calculateRays()
{
	float vAngle = fov;
	float hAngle = 2 * atan(aspect * tan(vAngle / 2));

	Vector3 locX;
	Vector3 locY;
	Vector3 locZ = dir;

	getLocal(locX, locY, locZ);

	ray00 = dir;
	ray00 = rotAxis(locY, -hAngle / 2, ray00, Vector3(0, 0, 0));
	ray00 = rotAxis(locX, -vAngle / 2, ray00, Vector3(0, 0, 0));

	ray10 = dir;
	ray10 = rotAxis(locY, hAngle / 2, ray10, Vector3(0, 0, 0));
	ray10 = rotAxis(locX, -vAngle / 2, ray10, Vector3(0, 0, 0));

	ray01 = dir;
	ray01 = rotAxis(locY, -hAngle / 2, ray01, Vector3(0, 0, 0));
	ray01 = rotAxis(locX, vAngle / 2, ray01, Vector3(0, 0, 0));

	ray11 = dir;
	ray11 = rotAxis(locY, hAngle / 2, ray11, Vector3(0, 0, 0));
	ray11 = rotAxis(locX, vAngle / 2, ray11, Vector3(0, 0, 0));

	Graphics::setView(eye, ray00, ray10, ray01, ray11);
}

void Camera::zoom(float amount)
{
	eye += dir * amount;

	calculateRays();
}

void Camera::pan(Vector2 amount)
{
	Vector3 locX;
	Vector3 locY;
	Vector3 locZ = dir;

	getLocal(locX, locY, locZ);

	eye += locX * amount.x;
	eye += locY * amount.y;

	calculateRays();
}

void Camera::orbit(Vector2 amount)
{
	Vector3 locX;
	Vector3 locY;
	Vector3 locZ = dir;

	getLocal(locX, locY, locZ);

	dir = rotAxis(locY, amount.x, dir.normalized(), eye).normalized();
	dir = rotAxis(locX, amount.y, dir.normalized(), eye).normalized();

	calculateRays();
}

Vector2 lastMouse;
bool wasMouseMiddleDown = false;
Vector2 start;
void Camera::update(sf::Window& window)
{
	//float yAngle = (int)sf::Keyboard::isKeyPressed(sf::Keyboard::A) - (int)sf::Keyboard::isKeyPressed(sf::Keyboard::D);
	//float xAngle = (int)sf::Keyboard::isKeyPressed(sf::Keyboard::W) - (int)sf::Keyboard::isKeyPressed(sf::Keyboard::S);

	calculateRays();

	/*
	Vector2 mouse = Vector2(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);

	Vector2 oldLastPos = lastMouse;
	lastMouse = mouse;
	mouse -= oldLastPos;

	if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
	{
		if (mouse.x != 0 || mouse.y != 0)
		{
			zoom(-mouse.y * 0.01);
		}
	}

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (mouse.x != 0 || mouse.y != 0)
		{
			pan(Vector2(mouse.x, -mouse.y) * 0.01);
		}
	}

	if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
	{
		if (mouse.x != 0 || mouse.y != 0)
		{
			orbit(Vector2(mouse.x, mouse.y) * 0.0001);
		}
	}
	*/
}