#pragma once

#include <deque>
#include <optional>

#include "../fsm.hpp"
#include "../level.hpp"
#include "../world.hpp"

#include "../gui/menu_bar.hpp"

namespace states {

// level editor
class edit : public state {
public:
	edit(resource& r);
	~edit();

	void update(fsm* sm, sf::Time dt);
	void process_event(sf::Event e);
	void imdraw(fsm* sm);

private:
	// for pixel scaling without lines
	sf::RenderTexture m_rt;
	sf::Sprite m_map;

	ImGui::AppMenuBar m_menu_bar;

	void draw(sf::RenderTarget&, sf::RenderStates) const;

	level m_level;										   // the level we are editing
	std::deque<std::vector<tilemap::diff>> m_undo_queue;   // all changes made
	std::deque<std::vector<tilemap::diff>> m_redo_queue;   // redo queue
	const int UNDO_MAX = 100;							   // maximum amount of undos

	level m_cursor;		// the level that just renders the cursor
	tilemap m_border;	// a completely static map used to render a border of blocks

	float m_level_size;	  // x/y pixel size of the level

	enum cursor_type {
		PENCIL = 0,
		FLOOD,
		STROKE,
	} m_cursor_type;
	const char* m_cursor_description(cursor_type c) const;

	char m_import_buffer[8192];

	sf::Texture& m_tiles;	// texture of all tiles
	sf::Texture& m_tools;	// texture of tools
	tile::tile_type m_selected_tile = tile::begin;

	// sets the tile on the map, false if the tile could not be placed, with an error output
	std::optional<tilemap::diff> m_set_tile(sf::Vector2i pos, tile::tile_type type, std::string& error);
	// recursively flood fills the map
	std::vector<tilemap::diff> m_flood_fill(sf::Vector2i pos, tile::tile_type type, tile::tile_type replacing, std::string& error);

	// for undoing entire pencil strokes at once
	std::vector<tilemap::diff> m_pencil_undo_queue;
	bool m_pencil_active;

	// draw straight lines
	std::vector<tilemap::diff> m_stroke_fill(sf::Vector2i pos, tile::tile_type type, std::string& error);
	sf::Vector2i m_stroke_start;
	bool m_stroke_active;
	tilemap m_stroke_map;	// for temporary rendering of stroke tool graphics

	sf::Vector2i m_old_mouse_tile;		  // the hovered tile last update, so we can reset it when the mouse moves
	sf::Vector2i m_update_mouse_tile();	  // update the editor cursor
	sf::Vector2i m_last_placed;			  // the position of the last clicked tile

	bool m_test_playing() const;
	std::unique_ptr<world> m_test_play_world;
	void m_toggle_test_play();

	std::string m_info_msg;

	void m_update_transforms();	  // update the transforms based on m_level_size
	float m_level_scale() const;

	void m_controls(fsm* sm);
	void m_block_picker(fsm* sm);
};

}
