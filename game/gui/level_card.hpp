#pragma once

#include <SFML/Graphics.hpp>
#include <stack>

#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "api.hpp"
#include "api_handle.hpp"
#include "gui/gif.hpp"
#include "tilemap.hpp"

class fsm;

namespace ImGui {

/* a frame that stores a small preview of an api-fetched level */
class ApiLevelTile {
public:
	ApiLevelTile(api::level& lvl, sf::Color bg = sf::Color(0xC8AD7FFF));

	void imdraw(fsm* sm);

private:
	sf::Color m_bg;
	api::level& m_lvl;
	std::optional<api::vote> m_last_vote;
	tilemap m_tmap;	  // tilemap generated from the level data
	sf::RenderTexture m_map_tex;

	api_handle<api::vote_response> m_vote_handle;

	api_handle<api::replay_search_response> m_lb_handle;

	const char* m_sort_opts[4];	  // api sortBy options
	int m_sort_selection;

	const char* m_order_opts[2];   // api order options
	int m_order_selection;

	//! pagination
	std::stack<int> m_cursor_log;	// logs cursors from each pagination
	int m_cpage() const;			// current page we're on
	bool m_last_page() const;		// are we on the last page
	void m_next_page();
	void m_prev_page();

	void m_gui_leaderboard_popup(fsm* sm);
	void m_update_query();

	api::replay_search_query m_last_query;	 // holds the query that we send to the api
};

}
