#include "api.hpp"

#include "auth.hpp"
#include "json.hpp"
#include "level.hpp"
#include "settings.hpp"

api::api()
	: m_cli(settings::get().server_url()) {
}

api &api::get() {
	static api instance;
	return instance;
}

std::future<api::response> api::download_level(const char *id) {
	return std::async([this, id]() -> api::response {
		try {
			int idnumber	 = std::stoi(std::string(id));
			std::string path = "/level/" + std::to_string(idnumber);
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
