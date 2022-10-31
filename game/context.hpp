#pragma once

#include "api.hpp"
#include "level.hpp"
#include "tilemap.hpp"

/* manages app context / global state */
class context {
public:
	static context& get();

	// level currently in the editor
	level& editor_level();
	const level& editor_level() const;

	api::search_query& search_query();
	const api::search_query& search_query() const;

private:
	context();
	context(const context&) = delete;
	context(context&&)		= delete;

	level m_editor_level;
	api::search_query m_query;
};
