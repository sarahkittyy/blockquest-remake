#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

#include "resource.hpp"
#include "tilemap.hpp"

// a level's data, with the ability to render previews
class level : public sf::Drawable, public sf::Transformable {
public:
	level(resource& r, int xs = 32, int ys = 32);

	tilemap& map();				  // get the map
	const tilemap& map() const;	  // get the map

	bool valid() const;	  // checks if this level is valid, i.e. has a start and end point

	// json serialization
	nlohmann::json serialize() const;
	void deserialize(const nlohmann::json& j);

	// fetches the tile the mouse is hovering over
	sf::Vector2i mouse_tile() const;

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// for rendering

	resource& m_r;	  // resource mgr
	tilemap m_tmap;	  // associated tilemap
};
