#pragma once
#include "Vector.h"
using namespace Vector;
class Screen
{
public:
	static void setScreenSize(Vector2 size);
	static Vector2 getScreenSize();

	static void setWindowPos(Vector2 size);
	static Vector2 getWindowPos();

	static double getDeltaTime();
	static void setDeltaTime(double time);
};

