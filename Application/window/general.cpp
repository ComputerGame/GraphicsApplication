#include "module.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
using namespace sf;

#include "settings.h"


void ModuleWindow::Init()
{
	Global->Add<RenderWindow>("window");
	auto stg = Global->Get<Settings>("settings");

	VideoMode mde = VideoMode::getDesktopMode();
	stg->Position = Vector2i(mde.width / 2 - stg->Size.x / 2, mde.height / 2 - stg->Size.y / 2);
	Create(stg->Fullscreen);

	Listeners();
}

void ModuleWindow::Update()
{
	auto wnd = Global->Get<RenderWindow>("window");
	auto stg = Global->Get<Settings>("settings");
		
	if(wnd->isOpen())
	{
		sf::Event evt;
		while (wnd->pollEvent(evt))
		{
			switch(evt.type)
			{
			case Event::KeyPressed:
				Event->Fire<Keyboard::Key>("InputKeyPressed", evt.key.code);
				break;
			case Event::KeyReleased:
				Event->Fire<Keyboard::Key>("InputKeyReleased", evt.key.code);
				break;
			case Event::MouseButtonPressed:
				Event->Fire<Mouse::Button>("InputMousePressed", evt.mouseButton.button);
				break;
			case Event::Closed:
				Close();
				break;
			case Event::Resized:
				{ Vector2u Size(evt.size.width, evt.size.height);
				stg->Size = Size;
				Event->Fire<Vector2u>("WindowResize", Size);
				break; }
			case Event::GainedFocus:
				Event->Fire("WindowFocusGained");
				break;
			case Event::LostFocus:
				Event->Fire("WindowFocusLost");
				break;
			case Event::TextEntered:
				char letter = static_cast<char>(evt.text.unicode);
				if(31 < letter && letter < 128)
					Event->Fire<char>("WindowKeyText", letter);
				break;
			}
		}
	}
}

void ModuleWindow::Listeners()
{
	Event->Listen("InputBindExit", [=]{
		Close();
	});

	Event->Listen("InputBindFullscreen", [=]{
		Create();
	});

	Event->Listen("SystemUpdated", [=]{
		Global->Get<RenderWindow>("window")->display();
	});

	Event->Listen("InputBindVsync", [=]{
		auto stg = Global->Get<Settings>("settings");
		stg->Verticalsync = !stg->Verticalsync;
		Global->Get<RenderWindow>("window")->setVerticalSyncEnabled(stg->Verticalsync);
	});
}
