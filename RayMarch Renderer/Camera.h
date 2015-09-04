#pragma once
#include "Graphics.h"
#include "SFML\Window.hpp"
class Camera
{
private:
	float aspect;
	float fov;

	Vector3 eye;
	Vector3 dir;

	Vector3 ray00;
	Vector3 ray10;
	Vector3 ray01;
	Vector3 ray11;

	void zoom(float amount);
	void pan(Vector2 amount);
	void orbit(Vector2 amount);

public:
	Camera();
	Camera(Vector3 eyePos, Vector3 lookDir, float aspect, float fov);
	~Camera();

	void calculateRays();

	void update(sf::Window& window);

	void setAspect(float newAspect) { aspect = newAspect; };
};

