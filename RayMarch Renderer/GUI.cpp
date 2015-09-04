#include "stdafx.h"
#include "GUI.h"
#include "SFML/Graphics.hpp"

sf::RenderWindow* window;

void GUI::loadScene()
{
	std::vector<std::string> paths = listNames("data\\scenes\\", ".scene");

	int index = 0;
	try
	{
		GUI::addScene(GUI::loadJson(paths.at(index)));
	}
	catch (std::out_of_range)
	{
		std::cout << "ERROR: Scene Index Out of Range" << std::endl;
		return;
	}
}

void GUI::OnRenderButtonClick()
{
	rendering = true;
	reload = true;
}

bool draggingImage = false;
Vector2 dragStart;
Vector2 imageStart;

void GUI::BeginImageDrag()
{
	draggingImage = true;
	dragStart = Vector2(sf::Mouse::getPosition(*window).x, sf::Mouse::getPosition(*window).y);
	imageStart = imageCentre;
}

void GUI::EndImageDrag()
{
	draggingImage = false;
}

void GUI::SetSamples()
{
	try
	{
		samples = std::stoi((std::string)entry->GetText());
	}
	catch (std::invalid_argument) 
	{
		samples = 0;
	}
}

GUI::GUI(sf::RenderWindow* Window)
{
	window = Window;

	imageCentre = Screen::getScreenSize() / 2;
	imageSize = Screen::getScreenSize();
	imageZoom = 1;

	renderButton = sfg::Button::Create("Render");
	renderButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::OnRenderButtonClick, this));

	entry = sfg::Entry::Create("128");
	entry->GetSignal(sfg::Entry::OnTextChanged).Connect(std::bind(&GUI::SetSamples, this));

	sideBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	sideBox->PackEnd(renderButton, true, true);
	sideBox->PackEnd(entry, true, true);

	sideWindow = sfg::Window::Create();
	sideWindow->SetTitle("");
	sideWindow->Add(sideBox);

	sideWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	sideWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - sideWindow->GetAllocation().width, 0, sideWindow->GetAllocation().width, Screen::getScreenSize().y));

	mainWindow = sfg::Window::Create();
	mainWindow->SetTitle("");

	mainWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	mainWindow->SetAllocation(sf::FloatRect(0, 0, Screen::getScreenSize().x - sideWindow->GetAllocation().width, Screen::getScreenSize().y));

	mainWindow->GetSignal(sfg::Window::OnMouseRightPress).Connect(std::bind(&GUI::BeginImageDrag, this));
	mainWindow->GetSignal(sfg::Window::OnMouseRightRelease).Connect(std::bind(&GUI::EndImageDrag, this));

	desktop.Add(sideWindow);
	desktop.Add(mainWindow);

	loadScene();
}

GUI::~GUI()
{

}

void GUI::handleEvent(sf::Event windowEvent)
{
	if (windowEvent.type == sf::Event::MouseWheelScrolled)
	{
		imageZoom += zoomStep * imageZoom * windowEvent.mouseWheelScroll.delta;
		imageZoom = glm::max(imageZoom, 0.0f);
	}

	desktop.HandleEvent(windowEvent);
}

void GUI::update(float deltaTime)
{
	if (draggingImage)
	{
		Vector2 mouse = Vector2(sf::Mouse::getPosition(*window).x, sf::Mouse::getPosition(*window).y);
		imageCentre = imageStart + (mouse - dragStart);
	}

	desktop.Update(deltaTime);
}

void GUI::display(sf::RenderWindow &target)
{
	sfgui.Display(target);

	sf::FloatRect r = mainWindow->GetAllocation();
	Graphics::Display(imageCentre, imageZoom, Vector2(r.left + 1, r.top + 1), Vector2(r.left + r.width - 1, r.top + r.height - 1));
}