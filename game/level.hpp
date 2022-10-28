#pragma once

#include <SFML/Graphics.hpp>

#include "resource.hpp"
#include "tilemap.hpp"

#include "api.hpp"

// a level's data, with the ability to render previews
class level : public sf::Drawable, public sf::Transformable {
public:
	level(resource& r, int xs = 32, int ys = 32);

	struct metadata {
		metadata(int id, std::string author, std::string title, std::string description);
		int id;
		std::string author;
		std::string title;
		std::string description;
		std::time_t createdAt;
		std::time_t updatedAt;
	};

	tilemap& map();				  // get the map
	const tilemap& map() const;	  // get the map

	bool valid() const;	  // checks if this level is valid, i.e. has a start and end point

	// fetches the tile the mouse is hovering over
	sf::Vector2i mouse_tile(sf::Vector2f mouse_pos) const;

	// does this level have metadata or is it local only
	bool has_metadata() const;
	metadata& get_metadata();
	const metadata& get_metadata() const;
	void clear_metadata();

	// load from api data
	void load_from_api(api::level data);

	void clear();	// reset this level

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;	// for rendering

	resource& m_r;	  // resource mgr
	tilemap m_tmap;	  // associated tilemap

	std::optional<metadata> m_metadata;	  // optional level metadata
};
