#pragma once

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>

#include "resource.hpp"
#include "tilemap.hpp"
#include "world.hpp"

// top level app class
class app {
public:
	app();
	int run();

	void configure_imgui_style();

private:
	sf::RenderWindow m_window;	 // app window
	resource m_r;				 // app resource mgr
};
