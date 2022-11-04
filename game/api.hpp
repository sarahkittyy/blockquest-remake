#pragma once

#include <ctime>
#include <future>
#include <optional>
#include <string>

#include "httplib.h"
#include "json.hpp"

class level;

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
		std::optional<int> myVote;
	};

	struct level_response {
		bool success;
		int code;
		std::optional<std::string> error;
		std::optional<api::level> level;
	};

	struct search_query {
		int cursor;
		int rows;
		int cols;
		std::string query;
		bool matchTitle;
		bool matchDescription;
		std::string sortBy;
		std::string order;
		bool operator==(const search_query& other) const;
		bool operator!=(const search_query& other) const;
	};

	struct search_response {
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

	std::future<level_response> upload_level(::level l, const char* title, const char* description, bool override = false);
	std::future<level_response> download_level(int id);
	std::future<vote_response> vote_level(api::level lvl, vote v);

	// get the current app version
	const char* version() const;

	// is this application up-to-date
	std::future<update_response> is_up_to_date();

	std::future<search_response> search_levels(search_query q);

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
