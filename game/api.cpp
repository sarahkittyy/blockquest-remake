#include "api.hpp"

#include "auth.hpp"
#include "json.hpp"
#include "level.hpp"
#include "settings.hpp"

#include <cstdlib>

#define STRINGIFY(s) #s
#define QUOTE(s) STRINGIFY(s)

api::api()
	: m_cli(settings::get().server_url()),
	  m_gh_cli("https://api.github.com") {
}

api &api::get() {
	static api instance;
	return instance;
}

std::future<api::search_response> api::search_levels(api::search_query q) {
	return std::async([this, q]() -> api::search_response {
		try {
			nlohmann::json body;
			body["cursor"] = q.cursor;
			body["limit"]  = q.rows * q.cols;
			if (q.query.length() != 0) body["query"] = q.query;
			body["matchTitle"]		 = q.matchTitle;
			body["matchDescription"] = q.matchDescription;
			body["sortBy"]			 = q.sortBy;
			body["order"]			 = q.order;

			if (auto res = m_cli.Post("/level/search", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					search_response rsp;
					rsp.cursor	= result["cursor"].get<int>();
					rsp.success = true;
					for (auto &level_json : result["levels"]) {
						api::level l{
							.id			 = level_json["id"].get<int>(),
							.author		 = level_json["author"],
							.code		 = level_json["code"],
							.title		 = level_json["title"],
							.description = level_json["description"],
							.createdAt	 = level_json["createdAt"].get<std::time_t>(),
							.updatedAt	 = level_json["updatedAt"].get<std::time_t>(),
							.downloads	 = level_json["downloads"].get<int>()
						};
						rsp.levels.push_back(l);
					}
					return rsp;
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error";
					}
				}
			} else {
				throw "Could not connect to server";
			}
		} catch (const char *e) {
			return {
				.success = false,
				.error	 = e
			};
		} catch (std::exception &e) {
			return {
				.success = false,
				.error	 = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.error	 = "Unknown error."
			};
		}
	});
}

void api::ping_download(int id) {
	std::thread([this, id]() -> void {
		try {
			std::string path = "/level/" + std::to_string(id) + "/ping-download";
			m_cli.Get(path);
		} catch (...) {
		}
	}).detach();
}

std::future<api::response> api::download_level(int id) {
	return std::async([this, id]() -> api::response {
		try {
			std::string path = "/level/" + std::to_string(id);
			if (auto res = m_cli.Get(path)) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					nlohmann::json level = result["level"];
					api::level l{
						.id			 = level["id"].get<int>(),
						.author		 = level["author"],
						.code		 = level["code"],
						.title		 = level["title"],
						.description = level["description"],
						.createdAt	 = level["createdAt"].get<std::time_t>(),
						.updatedAt	 = level["updatedAt"].get<std::time_t>(),
						.downloads	 = level["downloads"].get<int>()
					};
					return {
						.success = true,
						.code	 = 200,
						.level	 = l,
					};
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error";
					}
				}
			} else {
				throw "Could not connect to server";
			}
		} catch (std::invalid_argument &e) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = "Invalid ID"
			};
		} catch (const char *e) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = e
			};
		} catch (std::exception &e) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = "Unknown error."
			};
		}
	});
}

std::future<api::response> api::upload_level(::level l, const char *title, const char *description, bool override) {
	return std::async([this, l, title, description, override]() -> api::response {
		if (!auth::get().authed()) {
			return {
				.success = false,
				.code	 = 401,
				.error	 = "You need to be logged in to upload a level.",
			};
		}
		try {
			nlohmann::json body;
			body["code"]		= l.map().save();
			body["title"]		= title;
			body["description"] = description;
			body["jwt"]			= auth::get().get_jwt().raw;
			std::string path	= override ? "/level/upload/confirm" : "/level/upload";
			if (auto res = m_cli.Post(path, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					nlohmann::json level = result["level"];
					api::level l{
						.id			 = level["id"].get<int>(),
						.author		 = level["author"],
						.code		 = level["code"],
						.title		 = level["title"],
						.description = level["description"],
						.createdAt	 = level["createdAt"].get<std::time_t>(),
						.updatedAt	 = level["updatedAt"].get<std::time_t>(),
						.downloads	 = level["downloads"].get<int>()
					};
					return {
						.success = true,
						.code	 = 200,
						.level	 = l,
					};
				} else {
					if (res->status == 409) {
						return {
							.success = false,
							.code	 = 409,
							.level	 = {}
						};
					} else if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error";
					}
				}
			} else {
				throw "Could not connect to server.";
			}
		} catch (const char *e) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = e
			};
		} catch (std::exception &e) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.code	 = -1,
				.error	 = "Unknown error."
			};
		}
	});
}

const char *api::version() const {
#if !defined(NDEBUG)
	static const char *v = std::getenv("APP_TAG");
	return v ? v : QUOTE(APP_TAG);
#elif defined(APP_TAG)
	return QUOTE(APP_TAG);
#else
	return "vUNKNOWN";
#endif
}

std::future<api::update_response> api::is_up_to_date() {
	std::string ctag(version());
	return std::async([this, ctag]() -> api::update_response {
		try {
			if (auto res = m_gh_cli.Get("/repos/sarahkittyy/blockquest-remake/releases/latest")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success		= true,
						.up_to_date		= ctag == result["tag_name"],
						.latest_version = result["tag_name"]
					};
				} else {
					return {
						.success = false,
						.error	 = "Invalid response from github"
					};
				}
			} else {
				throw "Could not connect to github api";
			}
		} catch (const char *e) {
			return {
				.success = false,
				.error	 = e,
			};
		} catch (std::exception &e) {
			return {
				.success = false,
				.error	 = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.error	 = "Unknown error."
			};
		}
	});
}

bool api::search_query::operator==(const search_query &other) const {
	return rows == other.rows &&
		   cols == other.cols &&
		   query == other.query &&
		   matchTitle == other.matchTitle &&
		   matchDescription == other.matchDescription &&
		   sortBy == other.sortBy &&
		   order == other.order;
}

bool api::search_query::operator!=(const search_query &other) const {
	return !(other == *this);
}
