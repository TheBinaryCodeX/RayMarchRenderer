#pragma once
#include "Graphics.h"
#include <SFML/Window.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
class NodeSystem
{
public:
	struct Node
	{
		struct Input
		{
			GLuint id;
			bool connected = false;

			sfg::Box::Ptr box;
			sfg::Box::Ptr entryBox;

			sfg::Entry::Ptr xEntry;
			sfg::Entry::Ptr yEntry;
			sfg::Entry::Ptr zEntry;
		};

		struct Output
		{
			GLuint id;
			bool connected = false;
		};

		sfg::Window::Ptr window;
		sfg::Box::Ptr mainBox;
		sfg::Box::Ptr inBox;
		sfg::Box::Ptr outBox;

		std::vector<Input> inputs;
		std::vector<Output> outputs;

		std::string name;
		std::string displayName;

		Vector2 position;
		Vector2 viewPosition;

		Node()
		{

		}

		Node(sfg::Desktop* desktop)
		{
			name = "node_test";
			displayName = "Test Node";

			window = sfg::Window::Create();
			window->SetTitle(displayName);
			window->SetStyle(sfg::Window::Style::BACKGROUND | sfg::Window::Style::TITLEBAR);

			mainBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
			mainBox->SetSpacing(10);

			inBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
			outBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

			mainBox->PackEnd(inBox);
			mainBox->PackEnd(outBox);

			window->Add(mainBox);
		}

		void updatePosition()
		{
			sf::Vector2f wPos = window->GetAbsolutePosition();
			position += Vector2(wPos.x, wPos.y) - viewPosition;
			viewPosition = Vector2(wPos.x, wPos.y);
		}

		void addInput(std::string name)
		{
			Input input;

			input.xEntry = sfg::Entry::Create("0");
			input.yEntry = sfg::Entry::Create("0");
			input.zEntry = sfg::Entry::Create("0");

			input.xEntry->SetRequisition(sf::Vector2f(50, 0));
			input.yEntry->SetRequisition(sf::Vector2f(50, 0));
			input.zEntry->SetRequisition(sf::Vector2f(50, 0));

			input.entryBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);

			input.entryBox->PackEnd(input.xEntry, true, true);
			input.entryBox->PackEnd(input.yEntry, true, true);
			input.entryBox->PackEnd(input.zEntry, true, true);

			input.box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

			input.box->PackEnd(sfg::Label::Create(name));
			input.box->PackEnd(input.entryBox, true, false);

			inBox->PackEnd(input.box);

			inputs.push_back(input);
		}

		void addOutput(std::string name)
		{
			outBox->PackEnd(sfg::Label::Create(name));
		}
	};

private:
	sfg::Desktop* desktop;

	std::vector<Node> nodes;

	Vector2 viewPos;

	bool dragging = false;
	Vector2 viewStart;
	Vector2 currentDrag;

public:
	NodeSystem() {};
	NodeSystem(sfg::Desktop* desktop);
	~NodeSystem();

	void update();

	void Show(bool show);

	void setDragging(bool dragging);
	void setCurrentDrag(Vector2 v) { currentDrag = v; };
};

