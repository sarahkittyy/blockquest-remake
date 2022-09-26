#pragma once

#include <SFML/Graphics.hpp>

#include "animated_sprite.hpp"

// the controllable player
class player : public animated_sprite, public sf::Transformable {
public:
	player(sf::Texture& tex);

	void update();

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// sfml draw override
};
