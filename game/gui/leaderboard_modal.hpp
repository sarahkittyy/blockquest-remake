#pragma once

#include <imgui.h>
#include <stack>

#include "api.hpp"
#include "api_handle.hpp"

class fsm;
class user_modal;

class leaderboard_modal {
public:
	leaderboard_modal(api::level& lvl);
	~leaderboard_modal();

	void open();
	void imdraw(fsm* sm);

private:
	api::level m_lvl;

	static int m_next_id;
	int m_ex_id;   // for imgui id system

	api_handle<api::replay_search_response> m_api_handle;

	const char* m_sort_opts[4];	  // api sortBy options
	int m_sort_selection;

	const char* m_order_opts[2];   // api order options
	int m_order_selection;

	api::replay_search_query m_last_query;

	std::stack<int> m_cursor_log;	// logs cursors from each pagination

	std::unique_ptr<user_modal> m_user_modal;	// for opening user profiles

	int m_cpage() const;		// current page we're on
	bool m_last_page() const;	// are we on the last page
	void m_next_page();
	void m_prev_page();

	void m_update_query();
};
