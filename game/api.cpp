#include "api.hpp"

#include "auth.hpp"
#include "debug.hpp"
#include "level.hpp"
#include "replay.hpp"
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

api::level api::level_from_json(nlohmann::json lvl) {
	api::level l{
		.id			 = lvl["id"].get<int>(),
		.author		 = lvl["author"],
		.code		 = lvl.value("code", ""),
		.title		 = lvl["title"],
		.description = lvl["description"],
		.createdAt	 = lvl["createdAt"].get<std::time_t>(),
		.updatedAt	 = lvl["updatedAt"].get<std::time_t>(),
		.downloads	 = lvl["downloads"].get<int>(),
		.likes		 = lvl["likes"].get<int>(),
		.dislikes	 = lvl["dislikes"].get<int>(),
	};
	if (lvl.contains("myVote")) {
		l.myVote = lvl["myVote"].get<int>();
	}
	if (lvl.contains("record")) {
		l.record = lvl["record"].get<float>();
	}
	if (lvl.contains("myRecord")) {
		l.myRecord = lvl["myRecord"].get<float>();
	}
	return l;
}

nlohmann::json api::level_to_json(api::level lvl) {
	nlohmann::json ret;
	ret["id"]		   = lvl.id;
	ret["author"]	   = lvl.author;
	ret["code"]		   = lvl.code;
	ret["title"]	   = lvl.title;
	ret["description"] = lvl.description;
	ret["createdAt"]   = lvl.createdAt;
	ret["updatedAt"]   = lvl.updatedAt;
	ret["downloads"]   = lvl.downloads;
	ret["likes"]	   = lvl.likes;
	ret["dislikes"]	   = lvl.dislikes;
	if (lvl.myVote)
		ret["myVote"] = *lvl.myVote;
	if (lvl.record)
		ret["record"] = *lvl.record;
	if (lvl.myRecord)
		ret["myRecord"] = *lvl.myRecord;
	return ret;
}

std::future<api::level_search_response> api::search_levels(api::level_search_query q) {
	return std::async([this, q]() -> api::level_search_response {
		try {
			nlohmann::json body = q;
			body["limit"]		= q.rows * q.cols;
			auth::get().add_jwt_to_body(body);
			debug::log() << body.dump() << "\n";

			if (auto res = m_cli.Post("/level/search", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					level_search_response rsp;
					rsp.cursor	= result["cursor"].get<int>();
					rsp.success = true;
					for (auto &level_json : result["levels"]) {
						rsp.levels.push_back(level_from_json(level_json));
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

std::future<api::replay_upload_response> api::upload_replay(::replay rp) {
	return std::async([this, rp]() -> api::replay_upload_response {
		try {
			nlohmann::json body;
			body["replay"] = rp.serialize_b64();
			if (!debug::get().ndebug()) rp.save_to_file("misc/last_posted.rpl");
			auth::get().add_jwt_to_body(body);

			if (auto res = m_cli.Post("/replay/upload", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					replay_upload_response rsp;
					rsp.success = true;
					if (result.contains("newBest"))
						rsp.newBest = result["newBest"].get<float>();
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

std::future<api::replay_search_response> api::search_replays(int levelId, api::replay_search_query q) {
	return std::async([this, q, levelId]() -> api::replay_search_response {
		try {
			nlohmann::json body = q;
			auth::get().add_jwt_to_body(body);

			if (auto res = m_cli.Post("/replay/search/" + std::to_string(levelId), body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					replay_search_response rsp;
					rsp.cursor	= result["cursor"].get<int>();
					rsp.success = true;
					for (auto &replay_json : result["scores"]) {
						rsp.scores.push_back(replay_json.get<api::replay>());
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

std::future<api::level_response> api::download_level(int id) {
	return std::async([this, id]() -> api::level_response {
		try {
			std::string path = "/level/" + std::to_string(id);
			nlohmann::json body;
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post(path, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.code	 = 200,
						.level	 = level_from_json(result["level"]),
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

std::future<api::level_response> api::upload_level(::level l, const char *title, const char *description, bool override) {
	return std::async([this, l, title, description, override]() -> api::level_response {
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
			auth::get().add_jwt_to_body(body);
			std::string path = override ? "/level/upload/confirm" : "/level/upload";
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

std::future<api::vote_response> api::vote_level(api::level lvl, api::vote v) {
	return std::async([this, lvl, v]() -> api::vote_response {
		try {
			std::string url = "/level/";
			url += std::to_string(lvl.id);
			url += "/";
			url += v == vote::LIKE ? "like" : "dislike";
			nlohmann::json body;
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post(url, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.level	 = level_from_json(result["level"]),
					};
				} else {
					return {
						.success = false,
						.error	 = result["error"].get<std::string>()
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
#if !defined(NDEBUG)
	return std::async([this]() -> api::update_response {
		return {
			.success		= true,
			.up_to_date		= true,
			.latest_version = "vDEBUG"
		};
	});
#endif
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

bool api::level_search_query::operator==(const level_search_query &other) const {
	return rows == other.rows &&
		   cols == other.cols &&
		   query == other.query &&
		   matchTitle == other.matchTitle &&
		   matchDescription == other.matchDescription &&
		   sortBy == other.sortBy &&
		   order == other.order;
}

bool api::level_search_query::operator!=(const level_search_query &other) const {
	return !(other == *this);
}

bool api::replay_search_query::operator==(const replay_search_query &other) const {
	return sortBy == other.sortBy &&
		   order == other.order &&
		   limit == other.limit;
}

bool api::replay_search_query::operator!=(const replay_search_query &other) const {
	return !(other == *this);
}
