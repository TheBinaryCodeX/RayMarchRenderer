#include "stdafx.h"
#include "Screen.h"

Vector2 screenSize;
Vector2 windowPos;

static double deltaTime = 0;

void Screen::setScreenSize(Vector2 size)
{
	screenSize = size;
}

Vector2 Screen::getScreenSize()
{
	return screenSize;
}

void Screen::setWindowPos(Vector2 pos)
{
	windowPos = pos;
}

Vector2 Screen::getWindowPos()
{
	return windowPos;
}

double Screen::getDeltaTime()
{
	return deltaTime;
}

void Screen::setDeltaTime(double time)
{
	if (time != 0)
	{
		deltaTime = time;
	}
}