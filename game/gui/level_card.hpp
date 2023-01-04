#pragma once

#include <SFML/Graphics.hpp>
#include <stack>

#include "gui/comment_modal.hpp"
#include "gui/leaderboard_modal.hpp"
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
	static int m_next_id;
	int m_ex_id;

	sf::Color m_bg;
	api::level m_lvl;
	std::optional<api::vote> m_last_vote;
	tilemap m_tmap;	  // tilemap generated from the level data
	sf::RenderTexture m_map_tex;

	leaderboard_modal m_lb_modal;
	comment_modal m_comment_modal;

	api_handle<api::vote_response> m_vote_handle;
};

}
