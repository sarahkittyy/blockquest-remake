#pragma once

#include <vector>
#include "sio_client.h"

#include "api.hpp"
#include "api_handle.hpp"
#include "auth.hpp"
#include "settings.hpp"

class multiplayer {
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
		std::time_t updatedAt;
	};

	struct chat_message {
		std::string text;
		int authorId;
		std::time_t createdAt;
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

private:
	multiplayer();
	multiplayer(const multiplayer& other) = delete;
	multiplayer(multiplayer&& other)	  = delete;

	void m_on_open();
	void m_on_fail();
	void m_on_close();

	void m_configure_socket_listeners();

	bool m_chat_open;

	std::optional<std::string> m_room;
	std::unordered_map<int, player_data> m_player_data;
	std::unordered_map<int, player_state> m_player_state;
	player_data get_player_or_default(int uid) const;

	api_handle<api::multiplayer_token_response> m_token_handle;	  // for fetching the mp token to send to sio
	api_handle<bool> m_sio_connect_handle;

	std::vector<chat_message> m_chat_messages;
	char m_chat_input[256];

	// sio client
	sio::client m_h;
	state m_state;
	std::optional<std::string> m_mp_token;
};
