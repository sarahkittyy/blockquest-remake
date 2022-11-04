#pragma once

#include <SFML/Graphics.hpp>

#include "imgui-SFML.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "api.hpp"
#include "tilemap.hpp"

namespace ImGui {

/* a frame that stores a small preview of an api-fetched level */
class ApiLevelTile {
public:
	ApiLevelTile(api::level& lvl, sf::Color bg = sf::Color(0xC8AD7FFF));

	void imdraw(bool* download = nullptr);

private:
	sf::Color m_bg;
	api::level& m_lvl;
	std::optional<api::vote> m_last_vote;
	tilemap m_tmap;	  // tilemap generated from the level data
	sf::RenderTexture m_map_tex;

	std::future<api::vote_response> m_vote_future;
	std::optional<api::vote_response> m_vote_status;
};

}
