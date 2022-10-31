#include "context.hpp"

context::context()
	: m_editor_level(32, 32),
	  m_query({ .cursor = -1, .query = "", .matchTitle = true, .matchDescription = true, .sortBy = "id", .order = "desc" }) {
}

level& context::editor_level() {
	return m_editor_level;
}

const level& context::editor_level() const {
	return m_editor_level;
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
