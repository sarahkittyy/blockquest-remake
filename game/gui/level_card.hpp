#pragma once

#include <SFML/Graphics.hpp>
#include <stack>

#include "gui/comment_modal.hpp"
#include "gui/leaderboard_modal.hpp"
#include "gui/player_icon.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "api.hpp"
#include "api_handle.hpp"
#include "gui/gif.hpp"
#include "tilemap.hpp"

class fsm;
class user_modal;

/* a frame that stores a small preview of an api-fetched level */
class level_card {
public:
	level_card(api::level& lvl, sf::Color bg = sf::Color(0xC8AD7FFF));

	void imdraw(fsm* sm);

private:
	static int m_next_id;	// for imgui
	int m_ex_id;

	static int m_pinned_level_id;

	sf::Color m_bg;
	api::level m_lvl;
	std::optional<api::vote> m_last_vote;
	tilemap m_tmap;	  // tilemap generated from the level data
	sf::RenderTexture m_map_tex;

	leaderboard_modal m_lb_modal;
	comment_modal m_comment_modal;
	player_icon m_player_icon;
	std::unique_ptr<user_modal> m_user_modal;

	api_handle<api::vote_response> m_vote_handle;
	api_handle<api::response> m_pin_handle;
};

