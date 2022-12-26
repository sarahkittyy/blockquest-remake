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
	api_handle<api::comment_search_response> m_comment_handle;
	api_handle<api::comment_response> m_comment_post_handle;

	const char* m_lb_sort_opts[4];	 // api sortBy options
	int m_lb_sort_selection;

	const char* m_lb_order_opts[2];	  // api order options
	int m_lb_order_selection;

	const char* m_comment_sort_opts[1];	  // api sortBy options
	int m_comment_sort_selection;

	const char* m_comment_order_opts[2];   // api order options
	int m_comment_order_selection;

	char m_comment_buffer[251];

	//! pagination
	std::stack<int> m_lb_cursor_log;   // logs cursors from each pagination
	int m_lb_cpage() const;			   // current page we're on
	bool m_lb_last_page() const;	   // are we on the last page
	void m_lb_next_page();
	void m_lb_prev_page();
	std::stack<int> m_comment_cursor_log;	// logs cursors from each pagination
	int m_comment_cpage() const;			// current page we're on
	bool m_comment_last_page() const;		// are we on the last page
	void m_comment_next_page();
	void m_comment_prev_page();

	void m_gui_leaderboard_popup(fsm* sm);
	void m_gui_comments_popup(fsm* sm);
	void m_update_lb_query();
	void m_update_comment_query();

	api::replay_search_query m_last_replay_query;	  // holds the query that we send to the api
	api::comment_search_query m_last_comment_query;	  // holds the last comment query sent to the api
};

}
