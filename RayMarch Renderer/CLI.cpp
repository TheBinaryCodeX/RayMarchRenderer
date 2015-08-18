#include "stdafx.h"
#include "CLI.h"
#include <tinydir.h>
#include "json\json.h"

int* samples;
int* gridWidth;
int* gridHeight;

// Utility Functions
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
	for (int i = 0; i < scene["materials"].size(); i++)
	{
		Graphics::addMaterial(scene["materials"][i]);
	}

	for (int i = 0; i < scene["objects"].size(); i++)
	{
		Graphics::addObject(scene["objects"][i]);
	}
}

// Command Functions
void command_load_scene()
{
	std::vector<std::string> paths = listNames("data\\scenes\\", ".scene");
	for (int i = 0; i < paths.size(); i++)
	{
		std::cout << "[" << i << "] " << paths[i].substr(paths[i].find_last_of("\\") + 1) << std::endl;
	}

	std::string input;
	std::cin >> input;

	int index;
	try
	{
		index = std::stoi(input);
	}
	catch (std::invalid_argument)
	{
		std::cout << "ERROR: Invalid Number" << std::endl;
		return;
	}

	try
	{
		addScene(loadJson(paths.at(index)));
	}
	catch (std::out_of_range)
	{
		std::cout << "ERROR: Out of Range" << std::endl;
		return;
	}
}

void command_samples()
{
	std::cout << "Enter number:" << std::endl;

	std::string input;
	std::cin >> input;

	try
	{
		*samples = std::stoi(input);
	}
	catch (std::invalid_argument)
	{
		std::cout << "ERROR: Invalid Number" << std::endl;
		return;
	}
}

void command_grid_width()
{
	std::cout << "Enter number:" << std::endl;

	std::string input;
	std::cin >> input;

	try
	{
		*gridWidth = std::stoi(input);
	}
	catch (std::invalid_argument)
	{
		std::cout << "ERROR: Invalid Number" << std::endl;
		return;
	}
}

void command_grid_height()
{
	std::cout << "Enter number:" << std::endl;

	std::string input;
	std::cin >> input;

	try
	{
		*gridHeight = std::stoi(input);
	}
	catch (std::invalid_argument)
	{
		std::cout << "ERROR: Invalid Number" << std::endl;
		return;
	}
}

// Main Functions
void CLI::Init(int* samples_p, int* gridWidth_p, int* gridHeight_p)
{
	samples = samples_p;
	gridWidth = gridWidth_p;
	gridHeight = gridHeight_p;
}

void CLI::CheckInput(bool& rendering, bool& willSave)
{
	std::string input;
	std::cin >> input;

	if (input == "load_scene")
	{
		command_load_scene();
	}
	else if (input == "samples")
	{
		command_samples();
	}
	else if (input == "grid_width")
	{
		command_grid_width();
	}
	else if (input == "grid_height")
	{
		command_grid_height();
	}
	else if (input == "render")
	{
		rendering = true;
	}
	else if (input == "save")
	{
		willSave = true;
	}
}