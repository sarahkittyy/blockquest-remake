#include "multiplayer.hpp"

#include <functional>

#include "debug.hpp"

multiplayer& multiplayer::get() {
	static multiplayer instance;
	return instance;
}

multiplayer::multiplayer()
	: m_chat_open(false),
	  m_room({}),
	  m_state(state::DISCONNECTED),
	  m_mp_token({}) {
	m_h.set_open_listener(std::bind(&multiplayer::m_on_open, this));
	m_h.set_close_listener(std::bind(&multiplayer::m_on_close, this));
	m_h.set_fail_listener(std::bind(&multiplayer::m_on_fail, this));
}

void multiplayer::m_on_open() {
	debug::log() << "socket opened\n";
	// do not set connected until authed
	// m_state = state::CONNECTED;
}

void multiplayer::m_on_close() {
	debug::log() << "socket closed\n";
	m_state = state::DISCONNECTED;
}

void multiplayer::m_on_fail() {
	debug::log() << "socket failed\n";
	m_state = state::DISCONNECTED;
}

void multiplayer::connect() {
	if (m_token_handle.fetching()) return;
	if (!auth::get().authed()) return;
	m_state = state::CONNECTING;
	m_token_handle.reset(api::get().fetch_multiplayer_token());
}

void multiplayer::disconnect() {
	m_h.close();
	m_state	   = state::DISCONNECTED;
	m_mp_token = {};
	m_player_data.clear();
	m_player_state.clear();
	m_room = {};
}

void multiplayer::join(int level_id) {
	if (!ready()) return;
	if (m_room.has_value()) return;
	m_h.socket()->emit("join", sio::int_message::create(level_id));
	debug::log() << "joining room " << level_id << "\n";
	m_room = std::to_string(level_id);
}

void multiplayer::leave() {
	if (!m_room.has_value()) return;
	m_h.socket()->emit("leave");
	m_player_data.clear();
	m_player_state.clear();
	debug::log() << "leaving room " << m_room.value() << "\n";
	m_room		= {};
	m_chat_open = false;
}

std::optional<std::string> multiplayer::room() const {
	return m_room;
}

void multiplayer::update() {
	if (!auth::get().authed()) {
		disconnect();
	}

	m_token_handle.poll();
	if (m_token_handle.ready() && !m_token_handle.fetching()) {
		auto res   = m_token_handle.get();
		m_mp_token = res.token;
		if (m_mp_token.has_value()) {
			m_token_handle.reset();
			if (!m_sio_connect_handle.fetching()) {
				m_sio_connect_handle.reset(std::async([this]() -> bool {
					m_h.connect(settings::get().server_url());
					return m_h.opened();
				}));
			}
		}
	}

	m_sio_connect_handle.poll();
	if (m_sio_connect_handle.ready() && !m_sio_connect_handle.fetching()) {
		m_sio_connect_handle.reset();

		m_configure_socket_listeners();

		// authenticate
		auto ptr				= sio::object_message::create();
		ptr->get_map()["token"] = sio::string_message::create(m_mp_token.value());
		ptr->get_map()["id"]	= sio::int_message::create(auth::get().get_jwt().id);
		m_h.socket()->emit("authenticate", ptr);
	}
}

void multiplayer::m_configure_socket_listeners() {
	m_h.socket()->off_all();

	// error handling
	m_h.socket()->on("connection_error", [this](sio::event& ev) {
		debug::log() << "socket connect error: " << ev.get_message()->get_string() << "\n";
		disconnect();
	});
	m_h.socket()->on("warn", [this](sio::event& ev) {
		debug::log() << "socket warning: " << ev.get_message()->get_string() << "\n";
	});
	m_h.socket()->on("error", [this](sio::event& ev) {
		debug::log() << "socket error: " << ev.get_message()->get_string() << "\n";
		disconnect();
	});

	// joining and leaving (called when we do it too)
	m_h.socket()->on("joined", [this](sio::event& ev) {
		// player data
		auto data	   = ev.get_message()->get_map();
		player_data& d = m_player_data[data["id"]->get_int()];
		d.id		   = data["id"]->get_int();
		d.name		   = data["name"]->get_string();
		d.fill		   = sf::Color(data["fill"]->get_int());
		d.outline	   = sf::Color(data["outline"]->get_int());

		m_player_data[d.id] = d;

		// if it was us, update the room we're in
		if (d.id == auth::get().get_jwt().id) {
			m_room = data["room"]->get_string();
		}
	});
	m_h.socket()->on("left", [this](sio::event& ev) {
		// player id
		int id = ev.get_message()->get_int();
		// remove player from data and state
		m_player_data.erase(id);
		m_player_state.erase(id);
		// if it was us, update the room we're in
		if (id == auth::get().get_jwt().id) {
			m_room = {};
		}
	});

	// chat
	m_h.socket()->on("chat", [this](sio::event& ev) {
		auto data = ev.get_message()->get_map();
		chat_message msg;
		msg.text	  = data["text"]->get_string();
		msg.authorId  = data["authorId"]->get_int();
		msg.createdAt = data["createdAt"]->get_int();

		m_chat_messages.push_back(msg);
	});

	// auth
	m_h.socket()->on("authorized", [this](sio::event& ev) {
		debug::log() << "socket authenticated\n";
		m_state = state::CONNECTED;
	});

	m_h.socket()->on("unauthorized", [this](sio::event& ev) {
		debug::log() << "socket not valid\n";
		disconnect();
	});
}

multiplayer::player_data multiplayer::get_player_or_default(int uid) const {
	if (m_player_data.contains(uid)) {
		return m_player_data.at(uid);
	} else {
		player_data d;
		d.id	  = uid;
		d.fill	  = sf::Color::White;
		d.outline = sf::Color::Black;
		d.name	  = "Unknown";
		return d;
	}
}

void multiplayer::imdraw() {
	if (!ready()) return;
	if (!room().has_value()) return;

	std::string chat_title = "Chat - #";
	chat_title += room().value();
	chat_title += "###MPCHAT";

	if (ImGui::Begin(chat_title.c_str(), &m_chat_open)) {
		ImGui::BeginChild("ChatScrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
		for (auto& msg : m_chat_messages) {
			ImGui::TextWrapped("[%s] %s", get_player_or_default(msg.authorId).name.c_str(), msg.text.c_str());
		}
		ImGui::EndChild();
		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		if (ImGui::InputText("ChatInput", m_chat_input, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (m_chat_input[0]) {
				m_h.socket()->emit("chat", sio::string_message::create(m_chat_input));
				m_chat_input[0] = 0;
			}
			ImGui::SetKeyboardFocusHere(-1);
		}
		ImGui::PopItemWidth();
	}
	ImGui::End();
}

multiplayer::state multiplayer::get_state() const {
	return m_state;
}

bool multiplayer::ready() const {
	return m_state == state::CONNECTED;
}

void multiplayer::open_chat() {
	m_chat_open = true;
}
