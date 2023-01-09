#pragma once

#include <deque>
#include <optional>

#include "../fsm.hpp"
#include "../level.hpp"
#include "../world.hpp"

#include "../api.hpp"

#include "../api_handle.hpp"
#include "../gui/menu_bar.hpp"

namespace states {

// level editor
class edit : public state {
public:
	edit();
	edit(api::level lvl);
	edit(api::level lvl, replay rpl);
	~edit();

	void update(fsm* sm, sf::Time dt);
	void process_event(sf::Event e);
	void imdraw(fsm* sm);

private:
	// for pixel scaling without lines
	sf::RenderTexture m_rt;
	sf::Sprite m_map;

	sf::VertexArray m_grid;	  // gridlines
	sf::Transform m_grid_tf;

	sf::Sprite m_bg;

	menu_bar m_menu_bar;

	void draw(sf::RenderTarget&, sf::RenderStates) const;

	level& m_level();									   // just fetches the level from context
	const level& m_level() const;						   // just fetches the level from context
	std::deque<std::vector<tilemap::diff>> m_undo_queue;   // all changes made
	std::deque<std::vector<tilemap::diff>> m_redo_queue;   // redo queue
	const int UNDO_MAX = 100;							   // maximum amount of undos
	bool m_modified	   = false;

	level m_cursor;							 // the level that just renders the cursor
	std::optional<replay> m_loaded_replay;	 // the currently loaded replay file
	tilemap m_border;						 // a completely static map used to render a border of blocks

	float m_level_size;	  // x/y pixel size of the level

	enum cursor_type {
		PENCIL = 0,
		FLOOD,
		STROKE,
		HOLLOW_RECT,
		FILLED_RECT,
	} m_cursor_type;
	const char* m_cursor_description(cursor_type c) const;

	char m_import_buffer[8192];

	// buffers for level uploading
	char m_title_buffer[50];
	char m_description_buffer[256];
	int m_id;

	// future & status of level upload
	api_handle<api::level_response> m_upload_handle;
	api_handle<api::level_response> m_download_handle;
	api_handle<api::level_response> m_quickplay_handle;
	api_handle<api::replay_upload_response> m_upload_replay_handle;

	// can we upload the level currently on display
	bool m_is_current_level_ours() const;
	// loads a level downloaded from the api into the editor
	void m_load_api_level(api::level lvl);
	// clear the current level
	void m_clear_level();

	sf::Texture& m_tiles;	// texture of all tiles
	sf::Texture& m_tools;	// texture of tools
	tile::tile_type m_selected_tile		 = tile::begin;
	tile::tile_type m_selected_direction = tile::move_none;
	tile selected() const;

	// sets the tile on the map, false if the tile could not be placed, with an error output
	std::optional<tilemap::diff> m_set_tile(sf::Vector2i pos, tile::tile_type type, std::string& error);
	// recursively flood fills the map
	std::vector<tilemap::diff> m_flood_fill(sf::Vector2i pos, tile replacing, std::string& error);

	// for undoing entire pencil strokes at once
	std::vector<tilemap::diff> m_pencil_undo_queue;
	bool m_pencil_active;

	// draw straight lines
	std::vector<tilemap::diff> m_stroke_fill(sf::Vector2i pos, std::string& error);
	sf::Vector2i m_stroke_start;
	bool m_stroke_active;
	tilemap m_stroke_map;	// for temporary rendering of stroke tool graphics

	// draw rects
	std::vector<tilemap::diff> m_rect_fill(sf::Vector2i pos, bool hollow, std::string& error);
	sf::Vector2i m_rect_start;
	bool m_rect_active;
	tilemap m_rect_map;

	sf::Vector2i m_old_mouse_tile;		  // the hovered tile last update, so we can reset it when the mouse moves
	sf::Vector2i m_update_mouse_tile();	  // update the editor cursor
	sf::Vector2i m_last_placed;			  // the position of the last clicked tile

	bool m_mouse_pressed(sf::Mouse::Button btn) const;

	bool m_test_playing() const;
	std::unique_ptr<world> m_test_play_world;
	sf::Text m_timer_text;
	void m_toggle_test_play();

	std::unique_ptr<::replay> m_verification;

	std::string m_info_msg;

	void m_update_transforms();	  // update the transforms based on m_level_size
	float m_level_scale() const;

	void m_gui_controls(fsm* sm);
	void m_gui_block_picker(fsm* sm);
	void m_gui_level_info(fsm* sm);
	void m_gui_menu(fsm* sm);
	void m_gui_replay_submit(fsm* sm);
};

}
