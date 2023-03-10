#pragma once

#include <SFML/Graphics.hpp>

// for rendering above multiplayer player ghosts
class nametag : public sf::Drawable, public sf::Transformable {
public:
	nametag();

	void set_name(std::string name);

private:
	void draw(sf::RenderTarget& t, sf::RenderStates s) const;

	std::string m_name;
	sf::Text m_text;
	sf::RectangleShape m_border;
};
