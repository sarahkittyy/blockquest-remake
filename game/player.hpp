#pragma once

#include <SFML/Graphics.hpp>

#include "animated_sprite.hpp"

// the controllable player
class player : public sf::Drawable, public sf::Transformable {
public:
	player();

	void update();

	void set_outline_color(sf::Color outline);
	void set_fill_color(sf::Color fill);

	sf::Color get_outline_color();
	sf::Color get_fill_color();

	void set_animation(std::string name);
	void queue_animation(std::string name);

	sf::Vector2i size() const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// sfml draw override

	animated_sprite m_inner;
	animated_sprite m_outer;   // the player sprite's outline
};
