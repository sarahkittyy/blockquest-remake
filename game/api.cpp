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
#ifdef NO_VERIFY_CERTS
	m_cli.enable_server_certificate_verification(false);
#endif
}

api &api::get() {
	static api instance;
	return instance;
}

std::future<api::user_stats_response> api::fetch_user_stats(int id) {
	return std::async([this, id]() -> api::user_stats_response {
		try {
			nlohmann::json body = nlohmann::json::object();
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post("/users/" + std::to_string(id), body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					user_stats_response rsp;
					rsp.success = true;
					rsp.stats	= result.get<user_stats>();
					return rsp;
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error";
					}
				}
			} else {
				debug::log() << httplib::to_string(res.error()) << "\n";
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

std::future<api::user_stats_response> api::fetch_user_stats(std::string name) {
	return std::async([this, name]() -> api::user_stats_response {
		try {
			nlohmann::json body = nlohmann::json::object();
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post("/users/name/" + name, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					user_stats_response rsp;
					rsp.success = true;
					rsp.stats	= result.get<user_stats>();
					return rsp;
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error";
					}
				}
			} else {
				debug::log() << httplib::to_string(res.error()) << "\n";
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

std::future<api::level_search_response> api::search_levels(api::level_search_query q) {
	return std::async([this, q]() -> api::level_search_response {
		try {
			nlohmann::json body = q;
			body["limit"]		= q.rows * q.cols;
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post("/level/search", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					level_search_response rsp;
					rsp.cursor	= result["cursor"].get<int>();
					rsp.success = true;
					for (auto &level_json : result["levels"]) {
						rsp.levels.push_back(level_json.get<api::level>());
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
				debug::log() << httplib::to_string(res.error()) << "\n";
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

std::future<api::comment_response> api::post_comment(int levelId, std::string comment) {
	return std::async([this, levelId, comment]() -> api::comment_response {
		try {
			nlohmann::json body;
			body["comment"] = comment;
			auth::get().add_jwt_to_body(body);

			if (auto res = m_cli.Post("/comments/new/" + std::to_string(levelId), body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					comment_response rsp;
					rsp.success = true;
					rsp.code	= res->status;
					rsp.comment = result["comment"].get<api::comment>();
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

std::future<api::comment_search_response> api::get_comments(int levelId, api::comment_search_query q) {
	return std::async([this, q, levelId]() -> api::comment_search_response {
		try {
			nlohmann::json body = q;
			auth::get().add_jwt_to_body(body);

			if (auto res = m_cli.Post("/comments/level/" + std::to_string(levelId), body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					comment_search_response rsp;
					rsp.cursor	= result["cursor"].get<int>();
					rsp.success = true;
					for (auto &comment_json : result["comments"]) {
						rsp.comments.push_back(comment_json.get<api::comment>());
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
						.level	 = result["level"].get<api::level>()
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

std::future<api::level_response> api::quickplay_level() {
	return std::async([this]() -> api::level_response {
		try {
			if (auto res = m_cli.Get("/level/quickplay")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.code	 = 200,
						.level	 = result["level"].get<api::level>(),
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
					api::level l		 = level.get<api::level>();
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
			nlohmann::json body = nlohmann::json::object();
			auth::get().add_jwt_to_body(body);
			if (auto res = m_cli.Post(url, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.level	 = result["level"].get<api::level>(),
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

bool api::replay::operator==(const api::replay &other) const {
	return other.createdAt == createdAt &&	 //
		   other.updatedAt == updatedAt &&
		   other.levelId == levelId &&
		   other.time == time &&
		   other.user == user &&
		   other.version == version;
}

bool api::replay::operator!=(const api::replay &other) const {
	return !(*this == other);
}

bool api::comment_search_query::operator==(const api::comment_search_query &other) const {
	return limit == other.limit &&
		   sortBy == other.sortBy &&
		   order == other.order;
}

bool api::comment_search_query::operator!=(const api::comment_search_query &other) const {
	return !(*this == other);
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

void to_json(nlohmann::json &j, const api::user_stats &s) {
	j = nlohmann::json{
		{ "id", s.id },
		{ "username", s.username },
		{ "createdAt", s.createdAt },
		{ "tier", s.tier },
		{ "count", s.count },
	};
	if (s.recentLevel) {
		j["recentLevel"] = *s.recentLevel;
	}
	if (s.recentScore) {
		j["recentScore"] = *s.recentScore;
	}
}

void from_json(const nlohmann::json &j, api::user_stats &s) {
	s.id		= j.at("id").get<int>();
	s.username	= j.at("username").get<std::string>();
	s.createdAt = j.at("createdAt").get<std::time_t>();
	s.tier		= j.at("tier").get<int>();
	s.count		= j.at("count").get<api::user_stat_counts>();
	if (j.contains("recentLevel")) {
		s.recentLevel = j.at("recentLevel").get<api::level>();
	}
	if (j.contains("recentScore")) {
		s.recentScore = j.at("recentScore").get<api::replay>();
	}
	if (j.contains("recentScoreLevel")) {
		s.recentScoreLevel = j.at("recentScoreLevel").get<api::level>();
	}
}

void from_json(const nlohmann::json &j, api::level &l) {
	l.id		  = j["id"].get<int>();
	l.author	  = j["author"];
	l.code		  = j.value("code", "");
	l.title		  = j["title"];
	l.description = j["description"];
	l.createdAt	  = j["createdAt"].get<std::time_t>();
	l.updatedAt	  = j["updatedAt"].get<std::time_t>();
	l.downloads	  = j["downloads"].get<int>();
	l.comments	  = j["comments"].get<int>();
	l.likes		  = j["likes"].get<int>();
	l.dislikes	  = j["dislikes"].get<int>();
	if (j.contains("myVote")) {
		l.myVote = j["myVote"].get<int>();
	}
	if (j.contains("record")) {
		l.record = j["record"].get<api::level_record>();
	}
	if (j.contains("records")) {
		l.records = j["records"].get<int>();
	}

	if (j.contains("myRecord")) {
		l.myRecord = j["myRecord"].get<api::level_record>();
	}
}

void to_json(nlohmann::json &j, const api::level &l) {
	j["id"]			 = l.id;
	j["author"]		 = l.author;
	j["code"]		 = l.code;
	j["title"]		 = l.title;
	j["description"] = l.description;
	j["createdAt"]	 = l.createdAt;
	j["updatedAt"]	 = l.updatedAt;
	j["downloads"]	 = l.downloads;
	j["comments"]	 = l.comments;
	j["likes"]		 = l.likes;
	j["dislikes"]	 = l.dislikes;
	if (l.myVote)
		j["myVote"] = *l.myVote;
	if (l.record)
		j["record"] = *l.record;
	if (l.myRecord)
		j["myRecord"] = *l.myRecord;
	j["records"] = l.records;
}
