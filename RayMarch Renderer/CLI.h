#pragma once
#include "Graphics.h"
class CLI
{
public:
	static void Init(int* samples_p, int* gridWidth_p, int* gridHeight_p);
	static void CheckInput(bool& rendering);
};

