#pragma once

#include <ctime>
#include <future>
#include <optional>
#include <string>

#include "httplib.h"

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
	};

	struct response {
		bool success;
		int code;
		std::optional<std::string> error;
		std::optional<api::level> level;
	};

	struct search_query {
		int cursor;
		int limit;
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

	std::future<response> upload_level(::level l, const char* title, const char* description, bool override = false);
	std::future<response> download_level(int id);

	std::future<search_response> search_levels(search_query q);

	// sends a download ping to be run when we fetch a level
	void ping_download(int id);

private:
	api();
	api(const api& other) = delete;
	api(api&& other)	  = delete;

	httplib::Client m_cli;
};
