#pragma once

#include <SFML/Graphics.hpp>

#include "player.hpp"

class player_icon : public sf::Transformable {
public:
	player_icon(int fill, int outline);
	player_icon(sf::Color fill, sf::Color outline);

	void set_fill_color(int fill);
	void set_outline_color(int outline);
	void set_fill_color(sf::Color fill);
	void set_outline_color(sf::Color outline);

	void set_view(sf::View v);

	void update();

	void imdraw(int xs, int ys);

	const sf::Texture& get_tex();
	player& get_player();

private:
	player m_player;
	sf::RenderTexture m_player_rt;
};
