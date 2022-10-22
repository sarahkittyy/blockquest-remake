#pragma once

#include <SFML/Graphics.hpp>
#include "imgui-SFML.h"
#include "imgui.h"

#include "fsm.hpp"
#include "resource.hpp"
#include "tilemap.hpp"
#include "world.hpp"

// top level app class
class app {
public:
	app(int argc, char** argv);
	int run();

	void configure_imgui_style();

private:
	sf::RenderWindow m_window;	 // app window
	resource m_r;				 // app resource mgr
	fsm m_fsm;					 // app state machine
};
