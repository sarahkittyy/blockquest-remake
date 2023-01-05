#pragma once

#include "api.hpp"
#include "level.hpp"
#include "tilemap.hpp"

#include <string>

/* manages app context / global state */
class context {
public:
	static context& get();

	// level currently in the editor
	level& editor_level();
	const level& editor_level() const;

	// the global context instance listens to events
	void process_event(sf::Event e);

	std::string save() const;
	void load(std::string data);
	bool save_to_file(std::string path) const;
	bool load_from_file(std::string path);

	int& fps_limit();
	const int& fps_limit() const;

	api::level_search_query& level_search_query();
	const api::level_search_query& level_search_query() const;

	api::replay_search_query& replay_search_query();
	const api::replay_search_query& replay_search_query() const;

	api::comment_search_query& comment_search_query();
	const api::comment_search_query& comment_search_query() const;

	float& music_volume();
	float& sfx_volume();

	// are we using alternate block-bros style controls?
	bool& alt_controls();

private:
	context();
	context(const context&) = delete;
	context(context&&)		= delete;

	level m_editor_level;
	api::level_search_query m_level_query;
	api::replay_search_query m_replay_query;
	api::comment_search_query m_comment_query;
	float m_sfx_volume;
	float m_music_volume;
	int m_fps_limit;
	bool m_use_alt_ctrls;
};
