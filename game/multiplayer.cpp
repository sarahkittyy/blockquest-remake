#include "multiplayer.hpp"

#include <functional>
#include <vector>

#include "debug.hpp"
#include "gui/player_icon.hpp"
#include "player.hpp"
#include "player_ghost.hpp"
#include "resource.hpp"
#include "util.hpp"

multiplayer& multiplayer::get() {
	static multiplayer instance;
	return instance;
}

multiplayer::player_state multiplayer::player_state::empty(int id) {
	return multiplayer::player_state{
		.id		   = id,
		.controls  = world::control_vars::empty,
		.anim	   = "stand",
		.updatedAt = util::get_time(),
	};
}

sio::message::ptr multiplayer::player_state::to_message() const {
	auto ptr					= sio::object_message::create();
	ptr->get_map()["id"]		= sio::int_message::create(auth::get().id());
	ptr->get_map()["controls"]	= controls.to_message();
	ptr->get_map()["anim"]		= sio::string_message::create(anim);
	ptr->get_map()["updatedAt"] = sio::int_message::create(updatedAt);
	return ptr;
}

multiplayer::player_state multiplayer::player_state::from_message(const sio::message::ptr& msg) {
	auto data = msg->get_map();
	multiplayer::player_state s;
	s.id		= data["id"]->get_int();
	s.controls	= world::control_vars::from_message(data["controls"]);
	s.anim		= data["anim"]->get_string();
	s.updatedAt = data["updatedAt"]->get_int();
	return s;
}

multiplayer::multiplayer()
	: m_chat_open(false),
	  m_players_open(false),
	  m_room({}),
	  m_state(state::DISCONNECTED),
	  m_mp_token({}) {
	m_h.set_open_listener(std::bind(&multiplayer::m_on_open, this));
	m_h.set_close_listener(std::bind(&multiplayer::m_on_close, this));
	m_h.set_fail_listener(std::bind(&multiplayer::m_on_fail, this));

	// default player icon
	m_player_renders[-1] = std::make_shared<player_icon>(sf::Color::White, sf::Color::Black);

	m_self_tag.set_name("Unnamed");
}

void multiplayer::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	if (!auth::get().authed()) return;
	s.transform *= getTransform();
	const int me = auth::get().id();
	for (auto& [uid, p] : m_player_chars) {
		if (uid == me) continue;
		t.draw(*p, s);
	}
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
	m_player_renders.clear();
	m_player_chars.clear();
	m_room = {};
}

void multiplayer::join(int level_id) {
	if (!ready()) return;
	if (m_room.has_value()) return;
	m_h.socket()->emit("join", sio::int_message::create(level_id));
	debug::log() << "joining room " << level_id << "\n";
	// m_room = std::to_string(level_id);
}

void multiplayer::leave() {
	m_h.socket()->emit("leave");
	m_player_data.clear();
	m_player_state.clear();
	m_player_renders.clear();
	m_player_chars.clear();

	m_room		= {};
	m_chat_open = false;
}

void multiplayer::emit_state(const player_state& state) {
	m_last_state = state;
}

std::unordered_map<int, multiplayer::player_state> multiplayer::get_states() const {
	return m_player_state;
}

std::optional<std::string> multiplayer::room() const {
	return m_room;
}

void multiplayer::update() {
	if (!auth::get().authed()) {
		disconnect();
	}

	if (ready() && auth::get().authed() && m_room.has_value() && m_state_clock.getElapsedTime() > m_state_update_interval) {
		m_h.socket()->emit("state_update", m_last_state.to_message());
		m_state_clock.restart();
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
		ptr->get_map()["id"]	= sio::int_message::create(auth::get().id());
		m_h.socket()->emit("authenticate", ptr);
	}

	if (!m_player_data_queue.empty()) {
		std::scoped_lock<std::mutex> lock(m_player_data_queue_mutex);
		for (auto& data : m_player_data_queue) {
			m_player_renders[data.id].reset(new player_icon(data.fill, data.outline));
		}
		m_player_data_queue.clear();
	}

	if (!m_player_state_flush_list.empty()) {
		std::scoped_lock<std::mutex> lock(m_player_state_flush_list_mutex);
		for (auto& id : m_player_state_flush_list) {
			if (!m_player_chars.contains(id)) {
				m_player_chars[id].reset(new player_ghost());
			}
			m_player_chars[id]->flush_data(m_get_player_data_or_default(id));
			m_player_chars[id]->flush_state(m_get_player_state_or_default(id));
		}
		m_player_state_flush_list.clear();
	}

	if (!m_player_erase_queue.empty()) {
		std::scoped_lock<std::mutex> lock(m_player_erase_queue_mutex);
		for (auto& id : m_player_erase_queue) {
			m_player_data.erase(id);
			m_player_state.erase(id);
			m_player_state_flush_list.erase(id);
			m_player_chars.erase(id);
			m_player_renders.erase(id);
		}
		m_player_erase_queue.clear();
	}

	for (auto& [uid, p] : m_player_chars) {
		p->update();
	}
}

nametag& multiplayer::get_self_tag() {
	return m_self_tag;
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
	m_h.socket()->on_error([this](const std::shared_ptr<sio::message>& err) {
		debug::log() << "socket error"
					 << "\n";
		disconnect();
	});

	// joining and leaving (called when we do it too)
	m_h.socket()->on("joined", [this](sio::event& ev) {
		// player data
		auto data = ev.get_message()->get_map();
		int id	  = data["id"]->get_int();
		player_data d;
		d.id	  = id;
		d.name	  = data["name"]->get_string();
		d.fill	  = sf::Color(data["fill"]->get_int());
		d.outline = sf::Color(data["outline"]->get_int());
		debug::log() << "[joined] " << d.name << "#" << d.id << "\n";

		m_update_player_data(d);
		m_update_player_state(player_state::empty(d.id));

		chat_message join_msg;
		join_msg.authorId  = -2;
		join_msg.color	   = sf::Color(0x77dd77ff);
		join_msg.createdAt = std::time(nullptr);
		join_msg.text	   = "[+] " + d.name + " joined room #" + data["room"]->get_string() + "!";
		m_chat_messages.push_back(join_msg);

		// if it was us, update the room we're in
		if (d.id == auth::get().id()) {
			m_room = data["room"]->get_string();
			// also update our nametag
			m_self_tag.set_name(auth::get().username());
		}
	});
	m_h.socket()->on("left", [this](sio::event& ev) {
		// player id
		int id = ev.get_message()->get_int();

		if (m_player_data.contains(id)) {
			chat_message leave_msg;
			leave_msg.authorId	= -2;
			leave_msg.color		= sf::Color(0xff6961ff);
			leave_msg.createdAt = std::time(nullptr);
			leave_msg.text		= "[-] " + m_player_data.at(id).name + " left room #" + m_room.value() + ".";
			m_chat_messages.push_back(leave_msg);
		}

		m_remove_player(id);

		// if it was us, update the room we're in
		if (id == auth::get().id()) {
			m_room = {};
		}
	});

	// game state
	m_h.socket()->on("data_update", [this](sio::event& ev) {
		auto msgs = ev.get_message()->get_vector();
		debug::log() << "[data_update] sz:" << msgs.size() << "\n";
		for (auto& msg : msgs) {
			auto data = msg->get_map();
			int id	  = data["id"]->get_int();
			player_data d;
			d.id	  = id;
			d.name	  = data["name"]->get_string();
			d.fill	  = sf::Color(data["fill"]->get_int());
			d.outline = sf::Color(data["outline"]->get_int());

			m_update_player_data(d);
		}
	});
	m_h.socket()->on("state_update", [this](sio::event& ev) {
		auto msgs = ev.get_message()->get_vector();
		for (auto& msg : msgs) {
			player_state st = player_state::from_message(msg);
			if (!m_player_data.contains(st.id)) continue;	// ignore deleted players
			m_update_player_state(st);
		}
	});

	// chat
	m_h.socket()->on("chat", [this](sio::event& ev) {
		auto data = ev.get_message()->get_map();
		chat_message msg;
		msg.text	  = data["text"]->get_string();
		msg.authorId  = data["authorId"]->get_int();
		msg.createdAt = data["createdAt"]->get_int();
		msg.color	  = sf::Color::White;

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

multiplayer::player_data multiplayer::m_get_player_data_or_default(int uid) const {
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

multiplayer::player_state multiplayer::m_get_player_state_or_default(int uid) const {
	if (m_player_state.contains(uid)) {
		return m_player_state.at(uid);
	} else {
		player_state d = player_state::empty(uid);
		d.id		   = uid;
		d.updatedAt	   = util::get_time();
		return d;
	}
}

void multiplayer::m_update_player_state(const player_state& state) {
	m_player_state[state.id] = state;

	std::scoped_lock<std::mutex> lock(m_player_state_flush_list_mutex);
	m_player_state_flush_list.insert(state.id);
}

void multiplayer::m_update_player_data(const player_data& data) {
	m_player_data[data.id] = data;

	debug::log() << "updating player data for " << data.id << "\n";

	std::scoped_lock<std::mutex> lock(m_player_data_queue_mutex);
	m_player_data_queue.push_back(data);
}

void multiplayer::m_remove_player(int id) {
	std::scoped_lock<std::mutex> lock(m_player_erase_queue_mutex);
	m_player_erase_queue.push_back(id);
}

void multiplayer::imdraw() {
	if (!ready()) return;
	if (!room().has_value()) return;

	std::string chat_title = "Chat - #";
	chat_title += room().value();
	chat_title += " - Players: ";
	chat_title += std::to_string(player_count());
	chat_title += "###MPCHAT";

	if (m_chat_open) {
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin(chat_title.c_str(), &m_chat_open);
		ImGui::BeginChild("ChatScrolling###ChatScrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
		int i = 0;
		for (auto& msg : m_chat_messages) {
			ImGui::PushID(i);
			if (m_player_renders.contains(msg.authorId)) {
				m_player_renders.at(msg.authorId)->imdraw(16, 16);
				ImGui::SameLine();
			} else if (msg.authorId != -2) {
				m_player_renders.at(-1)->imdraw(16, 16);
				ImGui::SameLine();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, msg.color);
			ImGui::TextWrapped("%s", msg.text.c_str());
			ImGui::PopStyleColor();
			ImGui::PopID();
			i++;
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
		ImGui::End();
	}
}

int multiplayer::player_count() const {
	return m_player_chars.size();
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
