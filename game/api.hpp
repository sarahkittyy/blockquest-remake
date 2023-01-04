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

	struct level_record {
		std::string user;
		float time;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::level_record, user, time);
	};

	struct level {
		int id;
		std::string author;
		std::string code;
		std::string title;
		std::string description;
		std::time_t createdAt;
		std::time_t updatedAt;
		int downloads;
		int comments;
		int likes;
		int dislikes;
		std::optional<level_record> record;
		std::optional<level_record> myRecord;
		int records;
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
		bool operator==(const replay& other) const;
		bool operator!=(const replay& other) const;
	};

	struct user_stub {
		int id;
		std::string name;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::user_stub, id, name);
	};

	struct comment {
		user_stub user;
		int id;
		int levelId;
		std::string text;
		std::time_t createdAt;
		std::time_t updatedAt;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::comment, user, id, levelId, text, createdAt, updatedAt);
	};

	struct comment_response {
		bool success;
		int code;
		std::optional<api::comment> comment;
		std::optional<std::string> error;
	};

	struct comment_search_query {
		int cursor		   = -1;
		int limit		   = 10;
		std::string sortBy = "id";
		std::string order  = "desc";
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::comment_search_query, cursor, limit, sortBy, order);
		bool operator==(const comment_search_query& other) const;
		bool operator!=(const comment_search_query& other) const;
	};

	struct comment_search_response {
		bool success;
		int code;
		std::optional<std::string> error;

		std::vector<api::comment> comments;
		int cursor;
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

	struct user_stat_counts {
		int levels;
		int votes;
		int comments;
		int scores;
		int records;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(api::user_stat_counts, levels, votes, comments, scores, records);
	};

	struct user_stats {
		int id;
		std::string username;
		std::time_t createdAt;
		int tier;
		user_stat_counts count;
		std::optional<api::replay> recentScore;
		std::optional<api::level> recentScoreLevel;
		std::optional<api::level> recentLevel;
	};

	struct user_stats_response {
		bool success;
		std::optional<std::string> error;
		std::optional<api::user_stats> stats;
	};

	std::future<level_response> upload_level(::level l, const char* title, const char* description, bool override = false);
	std::future<level_response> download_level(int id);
	std::future<level_response> quickplay_level();
	std::future<vote_response> vote_level(api::level lvl, vote v);

	std::future<api::comment_response> post_comment(int levelId, std::string comment);
	std::future<api::comment_search_response> get_comments(int levelId, api::comment_search_query q);

	std::future<api::replay_search_response> search_replays(int levelId, api::replay_search_query q);
	std::future<api::replay_upload_response> upload_replay(::replay rp);

	std::future<api::user_stats_response> fetch_user_stats(int id);

	// get the current app version
	const char* version() const;

	// is this application up-to-date
	std::future<update_response> is_up_to_date();

	std::future<level_search_response> search_levels(level_search_query q);

	// sends a download ping to be run when we fetch a level
	void ping_download(int id);

private:
	api();
	api(const api& other) = delete;
	api(api&& other)	  = delete;

	httplib::Client m_cli;
	httplib::Client m_gh_cli;
};

void to_json(nlohmann::json& j, const api::level& l);
void from_json(const nlohmann::json& j, api::level& l);
void to_json(nlohmann::json& j, const api::user_stats& s);
void from_json(const nlohmann::json& j, api::user_stats& s);
