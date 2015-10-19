#include "stdafx.h"
#include "NodeSystem.h"

NodeSystem::NodeSystem(sfg::Desktop* desktop)
{
	this->desktop = desktop;
	viewPos = Vector2(-200, -200);

	nodes.push_back(Node(desktop));
	nodes[0].addInput("test_input");
	nodes[0].addOutput("test_output");
}

NodeSystem::~NodeSystem()
{

}

void NodeSystem::update()
{
	if (dragging)
	{
		viewPos = viewStart - currentDrag;
	}

	for (int i = 0; i < nodes.size(); i++)
	{
		nodes[i].updatePosition();

		if (dragging)
		{
			nodes[i].viewPosition = nodes[i].position - viewPos;
			nodes[i].window->SetPosition(sf::Vector2f(nodes[i].viewPosition.x, nodes[i].viewPosition.y));
		}

		desktop->BringToFront(nodes[i].window);
	}
}

void NodeSystem::Show(bool show)
{
	if (show)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			desktop->Add(nodes[i].window);
		}
	}
	else
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			desktop->Remove(nodes[i].window);
		}
	}
}

void NodeSystem::setDragging(bool dragging)
{
	this->dragging = dragging;
	if (dragging)
	{
		viewStart = viewPos;
	}
}
