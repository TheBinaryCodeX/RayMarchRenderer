#pragma comment(linker, "/subsystem:windows")

#include "stdafx.h"
#include "Program.h"
#include "Screen.h"
#include "Graphics.h"
#include "Camera.h"
#include "CLI.h"
#include "GUI.h"
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
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>
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

	sf::RenderWindow renderWindow(sf::VideoMode(Screen::getScreenSize().x, Screen::getScreenSize().y, 32), "RayMarch Renderer", sf::Style::Titlebar | sf::Style::Close);
	renderWindow.setActive();

	GUI gui = GUI(&renderWindow);

	renderWindow.resetGLStates();

	Graphics::setImageSize(Vector2(1280, 720));
	Graphics::Init();

	Camera camera = Camera(Vector3(-4, 4, -6), Vector3(4, -4, 6).normalized(), Graphics::getImageSize().x / Graphics::getImageSize().y, PI / 4);

	int samples = 128;
	int currentSample = 0;

	int gridWidth = 4;
	int gridHeight = 4;

	int chunkWidth = Graphics::getImageSize().x / gridWidth;
	int chunkHeight = Graphics::getImageSize().y / gridHeight;

	int x = ceil((float)gridWidth / 2.0) - 1;
	int y = ceil((float)gridHeight / 2.0) - 1;
	Vector2 dir = Vector2(-1, 0);

	int squaresPassed = 0;
	int lastSquaresPassed = 0;
	int distCount = 0;

	bool willSave = false;

	bool wasRendering = false;
	bool rendering = false;

	double oldTime = 0;
	double newTime = 0;
	clock_t t = clock();
	while (renderWindow.isOpen())
	{
		sf::Event windowEvent;
		while (renderWindow.pollEvent(windowEvent))
		{
			gui.handleEvent(windowEvent);
			switch (windowEvent.type)
			{
			case sf::Event::Closed:
				renderWindow.close();
				break;
			}
		}

		rendering = gui.getRendering();

		if (gui.getReload())
		{
			// Reload
			samples = gui.getSamples();
			currentSample = 0;

			gridWidth = gui.getGridSize().x;
			gridHeight = gui.getGridSize().y;

			chunkWidth = Graphics::getImageSize().x / gridWidth;
			chunkHeight = Graphics::getImageSize().y / gridHeight;

			x = ceil((float)gridWidth / 2.0) - 1;
			y = ceil((float)gridHeight / 2.0) - 1;
			dir = Vector2(-1, 0);

			squaresPassed = 0;
			lastSquaresPassed = 0;
			distCount = 0;

			Graphics::Reload();

			camera.setAspect(Graphics::getImageSize().x / Graphics::getImageSize().y);
			camera.calculateRays();
		}

		if (rendering)
		{
			if (samples == 0)
			{
				if (squaresPassed < gridWidth * gridHeight)
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
					{
						rendering = false;
						gui.stopRendering();
					}

					Vector2 min = Vector2(x * chunkWidth, y * chunkHeight);
					Vector2 max = Vector2((x + 1) * chunkWidth, (y + 1) * chunkHeight);

					Graphics::Render(newTime, min, max, currentSample);

					std::cout << currentSample << std::endl;

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
				}
				else
				{
					x = ceil((float)gridWidth / 2.0) - 1;
					y = ceil((float)gridHeight / 2.0) - 1;
					dir = Vector2(-1, 0);

					squaresPassed = 0;
					lastSquaresPassed = 0;
					distCount = 0;

					currentSample++;
				}
			}
			else
			{
				if (squaresPassed < gridWidth * gridHeight)
				{
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
					{
						rendering = false;
						gui.stopRendering();
					}

					if (currentSample < samples)
					{
						Vector2 min = Vector2(x * chunkWidth, y * chunkHeight);
						Vector2 max = Vector2((x + 1) * chunkWidth, (y + 1) * chunkHeight);

						Graphics::Render(newTime, min, max, currentSample);

						currentSample++;

						//std::cout << currentSample << "/" << samples << std::endl;
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
					if (willSave)
					{
						save();
						willSave = false;
					}

					rendering = false;
					gui.stopRendering();
				}
			}
		}
		//*/

		gui.update(newTime);

		renderWindow.clear();
		gui.display(renderWindow);
		renderWindow.display();

		oldTime = newTime;
		newTime = (double)(clock() - t) / (double)CLOCKS_PER_SEC;
		Screen::setDeltaTime(newTime - oldTime);
	}

	//Graphics::Cleanup();
}