#pragma once

#include <imgui.h>

#include "api.hpp"
#include "api_handle.hpp"

#include "gui/level_card.hpp"

class fsm;

class user_modal {
public:
	user_modal(int id);
	~user_modal();

	void open();
	void imdraw(fsm* sm);

private:
	int m_id;	// user id to fetch
	api_handle<api::user_stats_response> m_stats_handle;

	std::unique_ptr<ImGui::ApiLevelTile> m_recent_level_tile;
	std::unique_ptr<ImGui::ApiLevelTile> m_recent_score_tile;
	void m_fetch();
};
