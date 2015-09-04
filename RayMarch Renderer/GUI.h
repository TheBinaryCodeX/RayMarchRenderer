#pragma once
#include "Screen.h"
#include "Graphics.h"
#include "json/json.h"
#include <tinydir.h>
#include <SFML/Window.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

class GUI
{
private:
	sfg::SFGUI sfgui;
	sfg::Desktop desktop;
	std::shared_ptr<sfg::Window> mainWindow;
	std::shared_ptr<sfg::Window> sideWindow;
	std::shared_ptr<sfg::Box> sideBox;
	std::shared_ptr<sfg::Button> renderButton;
	std::shared_ptr<sfg::Entry> sampleNum;
	std::shared_ptr<sfg::Entry> imageWidth;
	std::shared_ptr<sfg::Entry> imageHeight;
	std::shared_ptr<sfg::Entry> gridWidth;
	std::shared_ptr<sfg::Entry> gridHeight;

	Vector2 imageCentre;
	Vector2 imageSize;
	float imageZoom;
	float zoomStep = 0.1;

	Vector2 gridSize = Vector2(4, 4);

	int samples = 128;
	bool rendering = false;
	bool reload = false;

	bool mouseInMain = false;

	void loadScene();

	void OnRenderButtonClick();
	void BeginImageDrag();
	void EndImageDrag();
	void SetImageWidth();
	void SetImageHeight();
	void SetSamples();
	void SetGridWidth();
	void SetGridHeight();
	void MouseEnterMain() { mouseInMain = true; };
	void MouseLeaveMain() { mouseInMain = false; };

	// Helper Functions
	std::vector<std::string> listNames(std::string path, std::string fileType)
	{
		std::vector<std::string> paths;

		tinydir_dir dir;

		// All
		tinydir_open(&dir, path.c_str());

		while (dir.has_next)
		{
			tinydir_file file;
			tinydir_readfile(&dir, &file);

			if (file.is_dir)
			{
				if (file.name[0] != '.')
				{
					std::vector<std::string> s = listNames(path + file.name + "\\", fileType);
					paths.insert(paths.end(), s.begin(), s.end());
				}
			}
			else
			{
				if (std::string(file.name).find(fileType) != std::string::npos)
				{
					paths.push_back(path + file.name);
				}
			}

			tinydir_next(&dir);
		}

		tinydir_close(&dir);

		return paths;
	}
	Json::Value loadJson(std::string path)
	{
		std::filebuf fb;
		if (fb.open(path, std::ios::in))
		{
			std::istream is(&fb);

			///*
			Json::Value root;
			Json::Reader reader;

			bool parsingSuccessful = reader.parse(is, root);
			if (parsingSuccessful)
			{
				fb.close();
				return root;
			}
			else
			{
				std::cout << "Failed to parse configuration\n" << reader.getFormattedErrorMessages() << std::endl;
				fb.close();
				return Json::Value();
			}
		}
		else
		{
			std::cout << "Failed to load file" << std::endl;
			return Json::Value();
		}
	}
	void addScene(Json::Value scene)
	{
		Graphics::clearScene();
		for (int i = 0; i < scene["materials"].size(); i++)
		{
			Graphics::addMaterial(scene["materials"][i]);
		}

		for (int i = 0; i < scene["objects"].size(); i++)
		{
			Graphics::addObject(scene["objects"][i]);
		}
	}

public:
	GUI(sf::RenderWindow* Window);
	~GUI();

	void handleEvent(sf::Event windowEvent);
	void update(float deltaTime);
	void display(sf::RenderWindow &target);

	int getSamples() { return samples; };
	bool getRendering() { return rendering; };
	bool getReload() { return reload; };
	Vector2 getGridSize() { return gridSize; };

	void stopRendering() { rendering = false; };
};

