#pragma once
#include "Graphics.h"
class CLI
{
public:
	struct Command
	{
		std::string name;
		void (*run)();

		Command()
		{

		}

		Command(std::string commandName, void (*runFunc)())
		{
			name = commandName;
			run = runFunc;
		}
	};

	struct Menu
	{
		std::string text;
		std::vector<Command> commands;

		void print()
		{
			system("cls");

			std::cout << "----------------" << std::endl;

			std::cout << text << std::endl;

			for (int i = 0; i < commands.size(); i++)
			{
				std::cout << commands[i].name << std::endl;
			}

			std::cout << "----------------" << std::endl;

			std::cout << std::endl;
		}

		void run(std::string commandName)
		{
			Command command;

			for (int i = 0; i < commands.size(); i++)
			{
				if (commands[i].name == commandName)
				{
					command = commands[i];
					break;
				}
			}

			if (command.name != "")
			{
				command.run();
			}
			else
			{
				std::cout << "Error: Unknown Input" << std::endl;
			}
		}
	};

public:
	static void Init();
	static void CheckInput(bool& rendering);
};

