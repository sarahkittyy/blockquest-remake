#pragma once

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "../fsm.hpp"

#include "gui/gif.hpp"
#include "gui/level_card.hpp"
#include "gui/menu_bar.hpp"

#include "api.hpp"

namespace states {

/* locally saved levels display */
class search : public state {
public:
	search();
	~search();

	void update(fsm* sm, sf::Time dt);
	void imdraw(fsm* sm);
	void process_event(sf::Event e);

private:
	menu_bar m_menu_bar;   // app menu bar

	// QUERY STUFF
	api::level_search_query& query();		// just fetches the query from context
	api::level_search_query m_last_query;	// holds the query that we send to the api

	const char* m_sort_opts[8];	  // api sortBy options
	int m_sort_selection;

	const char* m_order_opts[2];   // api order options
	int m_order_selection;

	int m_temp_rows, m_temp_cols;	// rows & columns of displayed levels
	// // // //

	std::stack<int> m_cursor_log;	// logs cursors from each pagination
	int m_cpage() const;			// current page we're on
	bool m_last_page() const;		// are we on the last page

	bool m_authed_last_frame;	// tracks the auth status so we can trigger an event when authed

	api_handle<api::level_search_response> m_query_handle;	 // holds the active query future
	api_handle<api::level_response> m_quickplay_handle;

	// handles automatic loading & caching to prevent unnecessary draws
	std::unordered_map<int, std::shared_ptr<level_card>> m_api_level_tile;
	level_card& m_gui_level_tile(api::level& lvl);

	void m_update_query();	 // sends the query to the api
	void m_next_page();
	void m_prev_page();

	bool m_searching() const;	// are we currently searching the api

	ImGui::Gif m_loading_gif;	// gif for when the results are loading

	void m_gui_query_options();	  // renders the search options window

	std::string m_menu_info;   // extra string that renders on the main menu bar
};

}
