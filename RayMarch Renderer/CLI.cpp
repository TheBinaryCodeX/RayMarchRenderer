#include "stdafx.h"
#include "CLI.h"
#include <tinydir.h>
#include "json\json.h"

CLI::Menu currentMenu;

CLI::Menu mainMenu;

CLI::Menu loadSceneMenu;

CLI::Menu addMenu;

// Root Menu Functions
void loadScene_function()
{
	currentMenu = loadSceneMenu;
}

void add_function()
{
	currentMenu = addMenu;
}

void set_function()
{
	std::cout << "SetFunc" << std::endl;
}

// Add Menu Functions
void addSphere_function()
{

}

void addBox_function()
{

}

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

void CLI::Init()
{
	mainMenu.text = "Main:";
	mainMenu.commands.push_back(Command("add", add_function));
	mainMenu.commands.push_back(Command("set", set_function));

	addMenu.text = "Load Scene:";
	addMenu.commands.push_back(Command("sphere", addSphere_function));
	addMenu.commands.push_back(Command("box", addBox_function));

	addMenu.text = "add:";
	addMenu.commands.push_back(Command("sphere", addSphere_function));
	addMenu.commands.push_back(Command("box", addBox_function));

	currentMenu = mainMenu;
}

void addScene(Json::Value scene)
{
	/*
	for (int i = 0; i < scene["materials"].size(); i++)
	{
		Json::Value mat = scene["materials"][i];

		Graphics::Material m;
		m.id = mat["id"].asInt();
		m.color = Vector3(mat["color"][0].asFloat(), mat["color"][1].asFloat(), mat["color"][2].asFloat());
		m.reflective = mat["reflective"].asBool();
		m.transmissive = mat["transmissive"].asBool();
		m.emissive = mat["emissive"].asBool();
		m.power = mat["power"].asFloat();
		m.rRoughness = mat["rRoughness"].asFloat();
		m.tRoughness = mat["tRoughness"].asFloat();
		m.ior = mat["ior"].asFloat();
		m.mixFact = mat["mixFact"].asFloat();
		Graphics::addMaterial(m);
	}
	*/

	for (int i = 0; i < scene["materials"].size(); i++)
	{
		Graphics::addMaterial(scene["materials"][i]);
	}

	for (int i = 0; i < scene["objects"].size(); i++)
	{
		Graphics::addObject(scene["objects"][i]);

		/*
		Json::Value obj = scene["objects"][i];

		Graphics::Object o;
		o.matID = obj["matID"].asInt();

		o.centre = Vector3(obj["centre"][0].asFloat(), obj["centre"][1].asFloat(), obj["centre"][2].asFloat());

		if (obj["type"].asString() == "sphere")
		{
			o.type = 0;
			o.radius = Vector3(obj["radius"].asFloat(), obj["radius"].asFloat(), obj["radius"].asFloat());
		}
		else if (obj["type"].asString() == "box")
		{
			o.type = 1;
			o.radius = Vector3(obj["radius"][0].asFloat(), obj["radius"][1].asFloat(), obj["radius"][2].asFloat());
		}

		Graphics::addObject(o);
		*/
	}
}

void CLI::CheckInput(bool& rendering)
{
	/*
	currentMenu.print();

	std::string input;
	std::cin >> input;

	currentMenu.run(input);
	*/

	std::string input;
	std::cin >> input;

	if (input == "load_scene")
	{
		std::vector<std::string> paths = listNames("data\\scenes\\", ".scene");
		for (int i = 0; i < paths.size(); i++)
		{
			std::cout << "[" << i << "] " << paths[i] << std::endl;
		}

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
	else if (input == "render")
	{
		rendering = true;
	}
}