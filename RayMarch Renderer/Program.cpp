#pragma comment(linker, "/subsystem:windows")

#include "stdafx.h"
#include "Program.h"
#include "Screen.h"
#include "Graphics.h"
#include "Camera.h"
#include "CLI.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <Windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <time.h>
#include <math.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <SOIL.h>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window.hpp>
#include "stdafx.h"

#define GLEW_STATIC
//#define SFML_STATIC

static void OpenConsole()
{
	int outHandle, errHandle, inHandle;
	FILE *outFile, *errFile, *inFile;
	AllocConsole();
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = 9999;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	outHandle = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	errHandle = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
	inHandle = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT);

	outFile = _fdopen(outHandle, "w");
	errFile = _fdopen(errHandle, "w");
	inFile = _fdopen(inHandle, "r");

	*stdout = *outFile;
	*stderr = *errFile;
	*stdin = *inFile;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);

	std::ios::sync_with_stdio();
}

const float PI = 3.141592653;

inline int sign(int n)
{
	return n < 0 ? -1 : 1;
}

void save()
{
	time_t t = time(0);
	struct tm now;
	localtime_s(&now, &t);

	char buffer[80];
	strftime(buffer, 80, "%Y-%m-%d_%H-%M", &now);

	std::string name = std::string(buffer) + ".bmp";

	Graphics::SaveImage("output\\" + name);
	std::cout << "Saved image as: " << name << std::endl;
}

int main()
{
	OpenConsole();

	Screen::setScreenSize(Vector2(1280, 720));

	sf::Window window(sf::VideoMode(Screen::getScreenSize().x, Screen::getScreenSize().y, 32), "RayMarch Renderer", sf::Style::Titlebar | sf::Style::Close);

	Graphics::Init();
	Graphics::Reload();

	Camera camera = Camera(Vector3(0, 4, -8), Vector3(0, -4, 8).normalized(), Screen::getScreenSize().x / Screen::getScreenSize().y, PI / 4);
	
	CLI::Init();

	int samples = 8192;
	int currentSample = 0;

	int gridWidth = 4;
	int gridHeight = 4;

	int chunkWidth = Screen::getScreenSize().x / gridWidth;
	int chunkHeight = Screen::getScreenSize().y / gridHeight;

	int x = gridWidth / 2 - 1;
	int y = gridHeight / 2 - 1;
	Vector2 dir = Vector2(-1, 0);

	int squaresPassed = 0;
	int lastSquaresPassed = 0;
	int distCount = 0;

	bool willSave = true;
	bool saved = false;

	bool willReload = false;

	bool rendering = false;

	double oldTime = 0;
	double newTime = 0;
	clock_t t = clock();
	while (window.isOpen())
	{
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent))
		{
			switch (windowEvent.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			}
		}

		if (!rendering)
		{
			CLI::CheckInput(rendering);

			// Reload
			if (rendering)
			{
				x = gridWidth / 2 - 1;
				y = gridHeight / 2 - 1;
				dir = Vector2(-1, 0);

				squaresPassed = 0;
				lastSquaresPassed = 0;
				distCount = 0;

				Graphics::Reload();

				camera.calculateRays();

				willReload = false;
			}
		}
		else
		{
			if (squaresPassed < gridWidth * gridHeight)
			{
				if (currentSample < samples)
				{
					Graphics::Render(newTime, Vector2(x * chunkWidth, y * chunkHeight), Vector2((x + 1) * chunkWidth, (y + 1) * chunkHeight), samples, currentSample, Vector2(chunkWidth, chunkHeight));
					window.display();

					currentSample++;
					
					std::cout << currentSample << "/" << samples << std::endl;
				}
				else
				{
					x -= gridWidth / 2;
					y -= gridHeight / 2;

					if (distCount * 2 == squaresPassed - lastSquaresPassed)
					{
						distCount++;
						lastSquaresPassed = squaresPassed;
						dir = Vector2(dir.y, -dir.x);
					}
					else if (distCount == squaresPassed - lastSquaresPassed)
					{
						dir = Vector2(dir.y, -dir.x);
					}
					squaresPassed++;

					x += dir.x;
					y += dir.y;

					x += gridWidth / 2;
					y += gridHeight / 2;

					currentSample = 0;
				}
			}
			else
			{
				if (willSave && !saved)
				{
					save();
					saved = true;
				}

				rendering = false;
			}

			oldTime = newTime;
			newTime = (double)(clock() - t) / (double)CLOCKS_PER_SEC;
			Screen::setDeltaTime(newTime - oldTime);
		}
	}

	//Graphics::Cleanup();
}