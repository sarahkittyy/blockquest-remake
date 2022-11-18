#pragma once

#include <ctime>
#include <future>
#include <optional>
#include <string>

#include "httplib.h"
#include "json.hpp"

class level;
class replay;

/* singleton class for making queries to the api */
class api {
public:
	static api& get();

	struct level {
		int id;
		std::string author;
		std::string code;
		std::string title;
		std::string description;
		std::time_t createdAt;
		std::time_t updatedAt;
		int downloads;
		int likes;
		int dislikes;
		std::optional<float> record;
		std::optional<float> myRecord;
		std::optional<int> myVote;
	};

	struct replay {
		std::string user;
		int levelId;
		float time;
		std::string version;
		std::string raw;
		std::time_t createdAt;
		std::time_t updatedAt;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::replay, user, levelId, time, version, raw, createdAt, updatedAt);
	};

	struct level_response {
		bool success;
		int code;
		std::optional<std::string> error;
		std::optional<api::level> level;
	};

	struct level_search_query {
		int cursor			  = -1;
		int rows			  = 4;
		int cols			  = 4;
		std::string query	  = "";
		bool matchTitle		  = true;
		bool matchDescription = true;
		bool matchAuthor	  = false;
		bool matchSelf		  = false;
		std::string sortBy	  = "likes";
		std::string order	  = "desc";
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::level_search_query, cursor, rows, cols, query, matchTitle, matchDescription, matchAuthor, matchSelf, sortBy, order);
		bool operator==(const level_search_query& other) const;
		bool operator!=(const level_search_query& other) const;
	};

	struct level_search_response {
		bool success;
		std::optional<std::string> error;

		std::vector<api::level> levels;
		int cursor;
	};

	struct update_response {
		bool success;
		std::optional<bool> up_to_date;
		std::optional<std::string> latest_version;
		std::optional<std::string> error;
	};

	enum class vote {
		LIKE,
		DISLIKE
	};

	struct vote_response {
		bool success;
		std::optional<std::string> error;
		std::optional<api::level> level;
	};

	struct replay_search_query {
		int cursor = -1;
		int limit  = 10;

		std::string sortBy = "time";
		std::string order  = "asc";

		bool operator==(const replay_search_query& other) const;
		bool operator!=(const replay_search_query& other) const;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::replay_search_query, cursor, limit, sortBy, order);
	};

	struct replay_search_response {
		bool success;
		std::optional<std::string> error;
		std::vector<api::replay> scores;
		int cursor;
	};

	struct replay_upload_response {
		bool success;
		std::optional<std::string> error;
		std::optional<float> newBest;
	};

	std::future<level_response> upload_level(::level l, const char* title, const char* description, bool override = false);
	std::future<level_response> download_level(int id);
	std::future<vote_response> vote_level(api::level lvl, vote v);

	std::future<api::replay_search_response> search_replays(int levelId, api::replay_search_query q);
	std::future<api::replay_upload_response> upload_replay(::replay rp);

	// get the current app version
	const char* version() const;

	// is this application up-to-date
	std::future<update_response> is_up_to_date();

	std::future<level_search_response> search_levels(level_search_query q);

	// sends a download ping to be run when we fetch a level
	void ping_download(int id);

	static api::level level_from_json(nlohmann::json lvl);
	static nlohmann::json level_to_json(api::level lvl);

private:
	api();
	api(const api& other) = delete;
	api(api&& other)	  = delete;

	httplib::Client m_cli;
	httplib::Client m_gh_cli;
};
