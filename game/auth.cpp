#include "auth.hpp"

#include "settings.hpp"

auth::auth()
	: m_cli(settings::get().server_url()) {
}

auth& auth::get() {
	static auth instance;
	return instance;
}

bool auth::authed() const {
	return m_jwt.has_value();
}

std::future<auth::response> auth::signup(std::string email, std::string username, std::string password) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, email, username, password]() -> auth::response {
		try {
			std::string server_url = settings::get().server_url();
			nlohmann::json body;
			body["name"] = username;
			body["email"] = email;
			body["password"] = password;
			if (auto res = m_cli.Post("/signup", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					std::string jwt = result["jwt"];
					m_jwt = auth::jwt{
						.raw = jwt,
					};
					return {
						.success = true
					};
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error.";
					}
				}
			} else {
				throw "Could not connect to server.";
			}
		} catch (const char* e) {
			return {
				.success = false,
				.error = e
			};
		} catch (std::exception& e) {
			return {
				.success = false,
				.error = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.error = "Unknown error."
			};
		}
	});
	// clang-format on
}

std::future<auth::response> auth::login(std::string email_or_username, std::string password) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, email_or_username, password]() -> auth::response {
		try {
			std::string server_url = settings::get().server_url();
			nlohmann::json body;
			body["username"] = email_or_username;
			body["password"] = password;
			if (auto res = m_cli.Post("/login", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					std::string jwt = result["jwt"];
					m_jwt = auth::jwt{
						.raw = jwt,
					};
					return {
						.success = true
					};
				} else {
					if (result.contains("error")) {
						throw std::runtime_error(result["error"]);
					} else {
						throw "Unknown server error.";
					}
				}
			} else {
				throw "Could not connect to server.";
			}
		} catch (const char* e) {
			return {
				.success = false,
				.error = e
			};
		} catch (std::exception& e) {
			return {
				.success = false,
				.error = e.what()
			};
		} catch (...) {
			return {
				.success = false,
				.error = "Unknown error."
			};
		}
	});
	// clang-format on
}

void auth::logout() {
	m_jwt = {};
}
