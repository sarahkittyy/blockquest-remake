#include "context.hpp"

#include <fstream>
#include "json.hpp"

context::context()
	: m_editor_level(32, 32),
	  m_query({ .cursor = -1, .query = "", .matchTitle = true, .matchDescription = true, .sortBy = "id", .order = "desc" }) {
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

	j["editor_level"] = m_editor_level.map().save();
	if (m_editor_level.has_metadata()) {
		auto md				 = m_editor_level.get_metadata();
		nlohmann::json& md_j = j["editor_level_metadata"];
		md_j["author"]		 = md.author;
		md_j["createdAt"]	 = md.createdAt;
		md_j["updatedAt"]	 = md.updatedAt;
		md_j["title"]		 = md.title;
		md_j["description"]	 = md.description;
		md_j["downloads"]	 = md.downloads;
		md_j["id"]			 = md.id;
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

	return j.dump();
}

void context::load(std::string data) {
	nlohmann::json j = nlohmann::json::parse(data);

	if (j.contains("editor_level_metadata")) {
		api::level md;
		nlohmann::json md_j = j["editor_level_metadata"];
		md.author			= md_j["author"];
		md.createdAt		= md_j["createdAt"];
		md.updatedAt		= md_j["updatedAt"];
		md.title			= md_j["title"];
		md.description		= md_j["description"];
		md.downloads		= md_j["downloads"];
		md.id				= md_j["id"];
		md.code				= j["editor_level"];
		m_editor_level.load_from_api(md);
	} else {
		m_editor_level.map().load(j["editor_level"]);
	}

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
