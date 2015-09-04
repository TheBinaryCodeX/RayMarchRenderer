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

void GUI::SetImageWidth()
{
	try
	{
		imageSize.x = std::stoi((std::string)imageWidth->GetText());
	}
	catch (std::invalid_argument) {}
	Graphics::setImageSize(imageSize);
}

void GUI::SetImageHeight()
{
	try
	{
		imageSize.y = std::stoi((std::string)imageHeight->GetText());
	}
	catch (std::invalid_argument) {}
	Graphics::setImageSize(imageSize);
}

void GUI::SetSamples()
{
	try
	{
		samples = std::stoi((std::string)sampleNum->GetText());
	}
	catch (std::invalid_argument) 
	{
		samples = 0;
	}
}

void GUI::SetGridWidth()
{
	try
	{
		gridSize.x = std::stoi((std::string)gridWidth->GetText());
	}
	catch (std::invalid_argument) {}
}

void GUI::SetGridHeight()
{
	try
	{
		gridSize.y = std::stoi((std::string)gridHeight->GetText());
	}
	catch (std::invalid_argument) {}
}

GUI::GUI(sf::RenderWindow* Window)
{
	window = Window;

	imageCentre = Screen::getScreenSize() / 2;
	imageSize = Screen::getScreenSize();
	imageZoom = 1;

	renderButton = sfg::Button::Create("Render");
	renderButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::OnRenderButtonClick, this));

	sampleNum = sfg::Entry::Create("128");
	sampleNum->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetSamples, this));

	sideBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	sideBox->PackEnd(renderButton, true, false);

	// Image Size
	auto imageSizeX = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 0);
	imageSizeX->PackEnd(sfg::Label::Create("X:"), false, false);
	imageWidth = sfg::Entry::Create("1920");
	imageWidth->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetImageWidth, this));
	imageSizeX->PackEnd(imageWidth, true, true);

	auto imageSizeY = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 0);
	imageSizeY->PackEnd(sfg::Label::Create("Y:"), false, false);
	imageHeight = sfg::Entry::Create("1080");
	imageHeight->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetImageHeight, this));
	imageSizeY->PackEnd(imageHeight, true, true);

	auto imageSizeBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	imageSizeBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	imageSizeBox->PackEnd(sfg::Label::Create("Image Size"), true, false);
	imageSizeBox->PackEnd(imageSizeX, true, false);
	imageSizeBox->PackEnd(imageSizeY, true, false);

	sideBox->PackEnd(imageSizeBox, true, false);

	// Samples
	auto sampleBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	sampleBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	sampleBox->PackEnd(sfg::Label::Create("Samples"), true, false);
	sampleBox->PackEnd(sampleNum, true, false);

	sideBox->PackEnd(sampleBox, true, false);

	// Grid Size
	auto gridSizeX = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 0);
	gridSizeX->PackEnd(sfg::Label::Create("X:"), false, false);
	gridWidth = sfg::Entry::Create("4");
	gridWidth->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetGridWidth, this));
	gridSizeX->PackEnd(gridWidth, true, true);

	auto gridSizeY = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 0);
	gridSizeY->PackEnd(sfg::Label::Create("Y:"), false, false);
	gridHeight = sfg::Entry::Create("4");
	gridHeight->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetGridHeight, this));
	gridSizeY->PackEnd(gridHeight, true, true);

	auto gridSizeBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	gridSizeBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	gridSizeBox->PackEnd(sfg::Label::Create("Render Grid Size"), true, false);
	gridSizeBox->PackEnd(gridSizeX, true, false);
	gridSizeBox->PackEnd(gridSizeY, true, false);

	sideBox->PackEnd(gridSizeBox, true, false);

	// Side Window
	sideWindow = sfg::Window::Create();
	sideWindow->SetTitle("");
	sideWindow->Add(sideBox);

	sideWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	sideWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - sideWindow->GetAllocation().width, 0, sideWindow->GetAllocation().width, Screen::getScreenSize().y));

	// Main Window
	mainWindow = sfg::Window::Create();
	mainWindow->SetTitle("");

	mainWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	mainWindow->SetAllocation(sf::FloatRect(0, 0, Screen::getScreenSize().x - sideWindow->GetAllocation().width, Screen::getScreenSize().y));

	mainWindow->GetSignal(sfg::Window::OnMouseRightPress).Connect(std::bind(&GUI::BeginImageDrag, this));
	mainWindow->GetSignal(sfg::Window::OnMouseRightRelease).Connect(std::bind(&GUI::EndImageDrag, this));

	mainWindow->GetSignal(sfg::Window::OnMouseEnter).Connect(std::bind(&GUI::MouseEnterMain, this));
	mainWindow->GetSignal(sfg::Window::OnMouseLeave).Connect(std::bind(&GUI::MouseLeaveMain, this));

	// Desktop
	desktop.Add(sideWindow);
	desktop.Add(mainWindow);

	loadScene();
}

GUI::~GUI()
{

}

void GUI::handleEvent(sf::Event windowEvent)
{
	if (windowEvent.type == sf::Event::MouseWheelScrolled && mouseInMain)
	{
		imageZoom += zoomStep * imageZoom * windowEvent.mouseWheelScroll.delta;
		imageZoom = glm::max(imageZoom, 0.0f);
	}

	desktop.HandleEvent(windowEvent);
}

void GUI::update(float deltaTime)
{
	reload = false;

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