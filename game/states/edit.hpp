#pragma once

#include "../fsm.hpp"
#include "../level.hpp"
#include "../world.hpp"

namespace states {

// level editor
class edit : public state {
public:
	edit(resource& r);
	~edit();

	void update(fsm* sm, sf::Time dt);

	void imdraw(fsm* sm);

private:
	void draw(sf::RenderTarget&, sf::RenderStates) const;

	level m_level;	 // the level we are editing

	level m_cursor;		// the level that just renders the cursor
	tilemap m_border;	// a completely static map used to render a border of blocks

	const float m_level_scale;	 // scale of the level in the editor

	enum {
		PENCIL = 0,
		FLOOD,
		STROKE,
	} m_cursor_type;
	const char* m_selected_cursor_description() const;

	char m_import_buffer[8192];

	sf::Texture& m_tiles;	// texture of all tiles
	sf::Texture& m_tools;	// texture of tools
	tile::tile_type m_selected_tile = tile::begin;

	// sets the tile on the map, false if the tile could not be placed, with an error output
	bool m_set_tile(sf::Vector2i pos, tile::tile_type type, std::string& error);
	// recursively flood fills the map
	bool m_flood_fill(sf::Vector2i pos, tile::tile_type type, tile::tile_type replacing, std::string& error);

	// draw straight lines
	bool m_stroke_fill(sf::Vector2i pos, tile::tile_type type, std::string& error);
	sf::Vector2i m_stroke_start;
	bool m_stroke_active;
	tilemap m_stroke_map;	// for temporary rendering of stroke tool graphics

	sf::Vector2i m_old_mouse_tile;		  // the hovered tile last update, so we can reset it when the mouse moves
	sf::Vector2i m_update_mouse_tile();	  // update the editor cursor

	bool m_test_playing() const;
	std::unique_ptr<world> m_test_play_world;
	void m_toggle_test_play();

	std::string m_last_debug_msg;
};

}
