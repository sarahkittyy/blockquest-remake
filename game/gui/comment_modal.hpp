#pragma once

#include <imgui.h>
#include <stack>

#include "api.hpp"
#include "api_handle.hpp"

class fsm;

class comment_modal {
public:
	comment_modal(api::level& lvl);
	~comment_modal();

	void open();
	void imdraw(fsm* sm);

private:
	api::level m_lvl;
	api_handle<api::comment_search_response> m_fetch_handle;
	api_handle<api::comment_response> m_post_handle;

	const char* m_sort_opts[1];	  // api sortBy options
	int m_sort_selection;

	const char* m_order_opts[2];   // api order options
	int m_order_selection;

	char m_comment_buffer[251];

	std::stack<int> m_cursor_log;	// logs cursors from each pagination
	int m_cpage() const;			// current page we're on
	bool m_last_page() const;		// are we on the last page
	void m_next_page();
	void m_prev_page();

	void m_update_query();

	api::comment_search_query m_last_query;	  // holds the last comment query sent to the api
};
