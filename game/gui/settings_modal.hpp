#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

#include "api.hpp"
#include "api_handle.hpp"
#include "player.hpp"
#include "settings.hpp"

class settings_modal {
public:
	settings_modal();

	void open();
	void imdraw(bool* update_icon = nullptr);

	void process_event(sf::Event e);

private:
	static int m_next_id;
	int m_ex_id;   // for imgui

	std::optional<key> m_listening_key;	  // key to listen to for changing controls

	const std::string m_modal_name;

	bool m_open;

	float m_outer_player_color[4];
	float m_inner_player_color[4];

	player m_preview_player;
	sf::RenderTexture m_preview_player_rt;

	api_handle<api::response> m_submit_color_handle;
};
