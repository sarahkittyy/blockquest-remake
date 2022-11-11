#include "context.hpp"

#include <fstream>
#include "json.hpp"

#include "api.hpp"
#include "debug.hpp"
#include "resource.hpp"
#include "settings.hpp"

context::context()
	: m_editor_level(32, 32),
	  m_query({ .cursor			  = -1,
				.rows			  = 4,
				.cols			  = 4,
				.query			  = "",
				.matchTitle		  = true,
				.matchDescription = true,
				.matchAuthor	  = false,
				.matchSelf		  = false,
				.sortBy			  = "id",
				.order			  = "desc" }),
	  m_sfx_volume(50.f),
	  m_music_volume(50.f) {
	load_from_file("bq-r.json");
}

level& context::editor_level() {
	return m_editor_level;
}

const level& context::editor_level() const {
	return m_editor_level;
}

float& context::music_volume() {
	return m_music_volume;
}

float& context::sfx_volume() {
	return m_sfx_volume;
}

std::string context::save() const {
	nlohmann::json j;

	if (m_editor_level.has_metadata()) {
		auto md					   = m_editor_level.get_metadata();
		j["editor_level_metadata"] = api::level_to_json(md);
	} else {
		j["editor_level"] = m_editor_level.map().save();
	}

	auto& q	   = m_query;
	j["query"] = q;

	j["screen_size"]["x"] = resource::get().window().getSize().x;
	j["screen_size"]["y"] = resource::get().window().getSize().y;

	j["screen_pos"]["x"] = resource::get().window().getPosition().x;
	j["screen_pos"]["y"] = resource::get().window().getPosition().y;

	j["controls"] = settings::get().get_key_map();

	j["volume"]["sfx"]	 = m_sfx_volume;
	j["volume"]["music"] = m_music_volume;

	return j.dump();
}

void context::load(std::string data) {
	nlohmann::json j = nlohmann::json::parse(data);

	if (j.contains("editor_level_metadata")) {
		api::level md = api::level_from_json(j["editor_level_metadata"]);
		if (md.code.empty()) {
			md.code = j.value("editor_level", "");
		}
		if (!md.code.empty())
			m_editor_level.load_from_api(md);
	} else {
		m_editor_level.map().load(j["editor_level"]);
	}

	if (j.contains("query")) {
		j["query"].get_to(m_query);
	}

	if (j.contains("screen_size")) {
		sf::RenderWindow& win = resource::get().window();
		sf::Vector2i nsz(j["screen_size"]["x"].get<unsigned>(),
						 j["screen_size"]["y"].get<unsigned>());
		win.setSize(sf::Vector2u(nsz));
	}
	if (j.contains("screen_pos")) {
		sf::RenderWindow& win = resource::get().window();
		sf::Vector2i npos(j["screen_pos"]["x"].get<int>(),
						  j["screen_pos"]["y"].get<int>());
		win.setPosition(npos);
	}
	if (j.contains("controls")) {
		settings::get().set_key_map(j["controls"]);
	}
	if (j.contains("volume")) {
		m_sfx_volume   = j["volume"].value("sfx", 50.f);
		m_music_volume = j["volume"].value("music", 50.f);
	}
}

bool context::save_to_file(std::string path) const {
	std::ofstream file(path);
	if (!file) return false;
	file << save();
	file.close();
	return true;
}

bool context::load_from_file(std::string path) {
	try {
		std::ifstream file(path);
		if (!file) return false;
		load(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
		file.close();
		return true;
	} catch (const std::exception& e) {
		debug::log() << "error fetching " << path << ": " << e.what() << "\n";
		return false;
	}
}

void context::process_event(sf::Event e) {
	switch (e.type) {
	default:
		break;
		// save on close
	case sf::Event::Closed:
		save_to_file("bq-r.json");
		break;
	}
}

api::search_query& context::search_query() {
	return m_query;
}

const api::search_query& context::search_query() const {
	return m_query;
}

context& context::get() {
	static context instance;
	return instance;
}
