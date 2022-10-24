#pragma once

#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <SFML/Graphics.hpp>

namespace ImGui {

class Gif {
public:
	Gif(sf::Texture& gif_atlas, int frame_ct, sf::Vector2i frame_sz, int fps = 20);

	void update();
	void draw(sf::Vector2i sz);

private:
	sf::Texture& m_atlas;	// texture atlas
	sf::Vector2f m_sz;		// size of one frame, from 0-1
	sf::Clock m_c;			// clock for updating frames
	sf::Time m_per_frame;	// time per frame

	int m_frame;
	int m_frame_ct;
	sf::Vector2i m_atlas_sz;   // size of the atlas in frames
};

}
