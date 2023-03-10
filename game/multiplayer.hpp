#pragma once

#include <set>
#include <vector>
#include "sio_client.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics.hpp>

#include "api.hpp"
#include "api_handle.hpp"
#include "auth.hpp"
#include "settings.hpp"

class player_ghost;
class player;
class player_icon;

class multiplayer : public sf::Drawable, public sf::Transformable {
public:
	static multiplayer& get();

	enum class state {
		DISCONNECTED = 0,
		CONNECTING,
		CONNECTED
	};

	struct player_data {
		int id;
		std::string name;
		sf::Color outline;
		sf::Color fill;
	};

	struct player_state {
		int id;
		float xp;
		float yp;
		float xv;
		float yv;
		float sx;
		float sy;
		std::string anim = "stand";
		int inputs;
		bool grounded;
		uint64_t updatedAt;
	};

	struct chat_message {
		std::string text;
		int authorId;
		std::time_t createdAt;
		sf::Color color;
	};

	// connect to the socket.io server
	void connect();
	// call once per frame, polls api handles
	void update();
	// disconnect from multiplayer
	void disconnect();
	// fetch the state of the multiplayer engine
	state get_state() const;

	// shortcut for get_state() == state::CONNECTED
	bool ready() const;

	void join(int level_id);   // join a level's room
	void leave();			   // leave the current room. does nothing if already disconnected.
	std::optional<std::string> room() const;

	void open_chat();
	void imdraw();

	// id field is ignored
	void emit_state(const player_state& state);
	std::unordered_map<int, player_state> get_states() const;

private:
	multiplayer();
	multiplayer(const multiplayer& other) = delete;
	multiplayer(multiplayer&& other)	  = delete;

	void draw(sf::RenderTarget& t, sf::RenderStates s) const;

	void m_on_open();
	void m_on_fail();
	void m_on_close();

	void m_configure_socket_listeners();

	bool m_chat_open;
	bool m_players_open;

	std::optional<std::string> m_room;
	std::unordered_map<int, player_data> m_player_data;
	std::unordered_map<int, player_state> m_player_state;
	player_data m_get_player_data_or_default(int uid) const;
	player_state m_get_player_state_or_default(int uid) const;

	// rendering contexts created in other threads are invalidated when the thread closes.
	// we queue up the data updates so that we can generate the player icons when needed
	std::vector<player_data> m_player_data_queue;
	// all the uids to flush ghosts for
	std::set<int> m_player_state_flush_list;

	void m_update_player_state(const player_state& state);
	void m_update_player_data(const player_data& data);

	sf::Clock m_state_clock;
	player_state m_last_state;

	std::unordered_map<int, std::shared_ptr<player_icon>> m_player_renders;	  // thumbnail renders of all players
	std::unordered_map<int, std::shared_ptr<player_ghost>> m_player_chars;	  // gameplay renders of all players

	api_handle<api::multiplayer_token_response> m_token_handle;	  // for fetching the mp token to send to sio
	api_handle<bool> m_sio_connect_handle;

	std::vector<chat_message> m_chat_messages;
	char m_chat_input[256];

	// sio client
	sio::client m_h;
	state m_state;
	std::optional<std::string> m_mp_token;
};
