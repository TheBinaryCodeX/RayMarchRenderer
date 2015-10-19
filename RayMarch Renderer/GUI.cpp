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

int mainWindowMode = 0;

void GUI::SwitchTabImage()
{
	if (mainWindowMode != 0)
	{
		matNS.Show(false);
		settingsWindow->Add(imageSettingsBox);
		settingsWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - settingsWindow->GetAllocation().width, 0, settingsWindow->GetAllocation().width, Screen::getScreenSize().y));
	}

	mainWindowMode = 0;
}

void GUI::SwitchTabObject()
{
	if (mainWindowMode != 1)
	{
		matNS.Show(false);
		settingsWindow->Remove(imageSettingsBox);
		settingsWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - settingsWindow->GetAllocation().width, 0, settingsWindow->GetAllocation().width, Screen::getScreenSize().y));
	}

	mainWindowMode = 1;
}

void GUI::SwitchTabMaterial()
{
	if (mainWindowMode != 2)
	{
		matNS.Show(true);
		settingsWindow->Remove(imageSettingsBox);
		settingsWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - settingsWindow->GetAllocation().width, 0, settingsWindow->GetAllocation().width, Screen::getScreenSize().y));
	}

	mainWindowMode = 2;
}

bool dragging = false;
Vector2 dragStart;
Vector2 imageStart;

void GUI::BeginDrag()
{
	dragging = true;
	dragStart = Vector2(sf::Mouse::getPosition(*window).x, sf::Mouse::getPosition(*window).y);

	if (mainWindowMode == 0)
	{
		imageStart = imageCentre;
	}
	else if (mainWindowMode == 2)
	{
		matNS.setDragging(true);
	}
}

void GUI::EndDrag()
{
	dragging = false;
	matNS.setDragging(false);
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

std::shared_ptr<sfg::Box> makeLabelInput(std::shared_ptr<sfg::Entry> inputBox, std::string labelText)
{
	auto box = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 0);
	box->PackEnd(sfg::Label::Create(labelText), false, false);
	box->PackEnd(inputBox, true, true);
	return box;
}

GUI::GUI(sf::RenderWindow* Window)
{
	window = Window;

	imageCentre = Screen::getScreenSize() / 2;
	imageSize = Screen::getScreenSize();
	imageZoom = 0.5;

	renderButton = sfg::Button::Create("Render");
	renderButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::OnRenderButtonClick, this));

	sampleNum = sfg::Entry::Create("128");
	sampleNum->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetSamples, this));

	imageSettingsBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	imageSettingsBox->PackEnd(renderButton, true, false);

	// Image Size
	imageWidth = sfg::Entry::Create("1920");
	imageWidth->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetImageWidth, this));

	imageHeight = sfg::Entry::Create("1080");
	imageHeight->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetImageHeight, this));

	auto imageSizeBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	imageSizeBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	imageSizeBox->PackEnd(sfg::Label::Create("Image Size"), true, false);
	imageSizeBox->PackEnd(makeLabelInput(imageWidth, "X:"), true, false);
	imageSizeBox->PackEnd(makeLabelInput(imageHeight, "Y:"), true, false);

	imageSettingsBox->PackEnd(imageSizeBox, true, false);

	// Samples
	auto sampleBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	sampleBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	sampleBox->PackEnd(sfg::Label::Create("Samples"), true, false);
	sampleBox->PackEnd(sampleNum, true, false);

	imageSettingsBox->PackEnd(sampleBox, true, false);

	// Grid Size
	gridWidth = sfg::Entry::Create("4");
	gridWidth->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetGridWidth, this));

	gridHeight = sfg::Entry::Create("4");
	gridHeight->GetSignal(sfg::Entry::OnLostFocus).Connect(std::bind(&GUI::SetGridHeight, this));

	auto gridSizeBox = sfg::Box::Create(sfg::Box::Orientation::VERTICAL, 0);
	gridSizeBox->SetAllocation(sf::FloatRect(0, 0, 0, 0));
	gridSizeBox->PackEnd(sfg::Label::Create("Render Grid Size"), true, false);
	gridSizeBox->PackEnd(makeLabelInput(gridWidth, "X:"), true, false);
	gridSizeBox->PackEnd(makeLabelInput(gridHeight, "Y:"), true, false);

	imageSettingsBox->PackEnd(gridSizeBox, true, false);

	// Settings Window
	settingsWindow = sfg::Window::Create();
	settingsWindow->SetTitle("");
	settingsWindow->Add(imageSettingsBox);

	settingsWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	settingsWindow->SetAllocation(sf::FloatRect(Screen::getScreenSize().x - settingsWindow->GetAllocation().width, 0, settingsWindow->GetAllocation().width, Screen::getScreenSize().y));

	// Tab Box
	auto tabBox = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL, 2);

	// Image Tab Button
	imageTabButton = sfg::Button::Create("Image");
	imageTabButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::SwitchTabImage, this));

	tabBox->PackEnd(imageTabButton, false, false);

	// Object Tab Button
	objectTabButton = sfg::Button::Create("Objects");
	objectTabButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::SwitchTabObject, this));

	tabBox->PackEnd(objectTabButton, false, false);

	// Material Tab Button
	materialTabButton = sfg::Button::Create("Materials");
	materialTabButton->GetSignal(sfg::Button::OnLeftClick).Connect(std::bind(&GUI::SwitchTabMaterial, this));

	tabBox->PackEnd(materialTabButton, false, false);

	// Tab Window
	tabWindow = sfg::Window::Create();
	tabWindow->SetTitle("");
	tabWindow->Add(tabBox);

	tabWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	tabWindow->SetId("tab_window");

	// Main Window
	mainWindow = sfg::Window::Create();
	mainWindow->SetTitle("");

	mainWindow->SetStyle(sfg::Window::Style::BACKGROUND);
	mainWindow->SetId("main_window");
	
	mainWindow->GetSignal(sfg::Window::OnMouseRightPress).Connect(std::bind(&GUI::BeginDrag, this));
	mainWindow->GetSignal(sfg::Window::OnMouseRightRelease).Connect(std::bind(&GUI::EndDrag, this));

	mainWindow->GetSignal(sfg::Window::OnMouseEnter).Connect(std::bind(&GUI::MouseEnterMain, this));
	mainWindow->GetSignal(sfg::Window::OnMouseLeave).Connect(std::bind(&GUI::MouseLeaveMain, this));

	// Desktop
	desktop.Add(settingsWindow);
	desktop.Add(tabWindow);
	desktop.Add(mainWindow);

	tabWindow->SetAllocation(sf::FloatRect(0, 0, Screen::getScreenSize().x - settingsWindow->GetAllocation().width, 30));
	mainWindow->SetAllocation(sf::FloatRect(0, tabWindow->GetAllocation().height, Screen::getScreenSize().x - settingsWindow->GetAllocation().width, Screen::getScreenSize().y - tabWindow->GetAllocation().height));

	desktop.SetProperty("Window#tab_window", "Gap", 2);
	desktop.SetProperty("Window#main_window", "BackgroundColor", sf::Color(55, 55, 55));

	matNS = NodeSystem(&desktop);

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

	if (dragging)
	{
		Vector2 mouse = Vector2(sf::Mouse::getPosition(*window).x, sf::Mouse::getPosition(*window).y);

		if (mainWindowMode == 0)
		{
			imageCentre = imageStart + (mouse - dragStart);
		}
		else if (mainWindowMode == 2)
		{
			matNS.setCurrentDrag(mouse - dragStart);
		}
	}

	desktop.Update(deltaTime);

	matNS.update();
}

void GUI::display(sf::RenderWindow &target)
{
	sfgui.Display(target);

	if (mainWindowMode == 0)
	{
		sf::FloatRect r = mainWindow->GetAllocation();
		Graphics::Display(imageCentre, imageZoom, Vector2(r.left + 1, r.top + 1), Vector2(r.left + r.width - 1, r.top + r.height - 1));
	}
}