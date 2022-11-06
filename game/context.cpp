#include "context.hpp"

#include <fstream>
#include "json.hpp"

#include "api.hpp"
#include "resource.hpp"
#include "settings.hpp"

context::context()
	: m_editor_level(32, 32),
	  m_query({ .cursor = -1, .rows = 4, .cols = 4, .query = "", .matchTitle = true, .matchDescription = true, .matchSelf = false, .sortBy = "id", .order = "desc" }) {
	load_from_file("bq-r.json");
}

level& context::editor_level() {
	return m_editor_level;
}

const level& context::editor_level() const {
	return m_editor_level;
}

std::string context::save() const {
	nlohmann::json j;

	if (m_editor_level.has_metadata()) {
		auto md					   = m_editor_level.get_metadata();
		j["editor_level_metadata"] = api::level_to_json(md);
	} else {
		j["editor_level"] = m_editor_level.map().save();
	}

	auto& q				   = m_query;
	nlohmann::json& jq	   = j["query"];
	jq["query"]			   = q.query;
	jq["cols"]			   = q.cols;
	jq["rows"]			   = q.rows;
	jq["matchTitle"]	   = q.matchTitle;
	jq["matchDescription"] = q.matchDescription;
	jq["sortBy"]		   = q.sortBy;
	jq["order"]			   = q.order;

	j["screen_size"]["x"] = resource::get().window().getSize().x;
	j["screen_size"]["y"] = resource::get().window().getSize().y;

	j["screen_pos"]["x"] = resource::get().window().getPosition().x;
	j["screen_pos"]["y"] = resource::get().window().getPosition().y;

	j["controls"] = settings::get().get_key_map();

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
		auto& q			   = m_query;
		nlohmann::json& jq = j["query"];
		q.query			   = jq["query"];
		q.cols			   = jq["cols"];
		q.rows			   = jq["rows"];
		q.matchTitle	   = jq["matchTitle"];
		q.matchDescription = jq["matchDescription"];
		q.sortBy		   = jq["sortBy"];
		q.order			   = jq["order"];
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
}

bool context::save_to_file(std::string path) const {
	std::ofstream file(path);
	if (!file) return false;
	file << save();
	file.close();
	return true;
}

bool context::load_from_file(std::string path) {
	std::ifstream file(path);
	if (!file) return false;
	load(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
	file.close();
	return true;
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
