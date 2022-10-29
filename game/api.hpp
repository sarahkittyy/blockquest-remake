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

	std::future<response> upload_level(::level l, const char* title, const char* description, bool override = false);
	std::future<response> download_level(int id);

	// sends a download ping to be run when we fetch a level
	void ping_download(int id);

private:
	api();
	api(const api& other) = delete;
	api(api&& other)	  = delete;

	httplib::Client m_cli;
};
