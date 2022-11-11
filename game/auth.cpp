#include "auth.hpp"

#include "debug.hpp"
#include "settings.hpp"
#include "util.hpp"

auth::auth()
	: m_cli(settings::get().server_url()) {
}

auth& auth::get() {
	static auth instance;
	return instance;
}

bool auth::authed(bool confirmed) const {
	return m_jwt.has_value() && time(nullptr) < m_jwt->exp && (!confirmed || m_jwt->confirmed);
}

std::string auth::username() const {
	return m_jwt ? m_jwt->username : "Not logged in";
}

int auth::tier() const {
	return m_jwt ? m_jwt->tier : -1;
}

auth::jwt auth::get_jwt() {
	return *m_jwt;
}

void auth::add_jwt_to_body(nlohmann::json& body) {
	if (m_jwt.has_value()) {
		body["jwt"] = get_jwt().raw;
	}
}

std::future<auth::response> auth::signup(std::string email, std::string username, std::string password) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, email, username, password]() -> auth::response {
		try {
			nlohmann::json body;
			body["name"] = username;
			body["email"] = email;
			body["password"] = password;
			if (auto res = m_cli.Post("/signup", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					std::string jwt = result["jwt"].get<std::string>();
					size_t first_sep = jwt.find_first_of('.');
					size_t second_sep = jwt.find_first_of('.', first_sep + 1);
					std::string payload_b64 = jwt.substr(first_sep + 1, second_sep - first_sep - 1);
					char* payload_ascii_bin = util::base64_decode(payload_b64.data());
					std::string payload_ascii(payload_ascii_bin);
					delete[] payload_ascii_bin;
					nlohmann::json payload_json = nlohmann::json::parse(payload_ascii);
					m_jwt = auth::jwt{
						.exp = payload_json["exp"].get<std::time_t>(),
						.username = payload_json["username"].get<std::string>(),
						.id = payload_json["id"].get<int>(),
						.confirmed = payload_json["confirmed"].get<bool>(),
						.tier = payload_json["tier"].get<int>(),
						.raw = jwt,
					};
					return {
						.success = true,
						.confirmed = m_jwt->confirmed
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
					std::string jwt = result["jwt"].get<std::string>();
					size_t first_sep = jwt.find_first_of('.');
					size_t second_sep = jwt.find_first_of('.', first_sep + 1);
					std::string payload_b64 = jwt.substr(first_sep + 1, second_sep - first_sep);
					char* payload_ascii_bin = util::base64_decode(payload_b64.data());
					std::string payload_ascii(payload_ascii_bin);
					delete[] payload_ascii_bin;
					nlohmann::json payload_json = nlohmann::json::parse(payload_ascii);
					m_jwt = auth::jwt{
						.exp = payload_json["exp"].get<std::time_t>(),
						.username = payload_json["username"].get<std::string>(),
						.id = payload_json["id"].get<int>(),
						.confirmed = payload_json["confirmed"].get<bool>(),
						.tier = payload_json["tier"].get<int>(),
						.raw = jwt,
					};
					return {
						.success = true,
						.confirmed = m_jwt->confirmed,
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

std::future<auth::response> auth::verify(int code) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, code]() -> auth::response {
		try {
			std::string url = "";
			url += "/verify/";
			url += std::to_string(code);
			nlohmann::json body;
			if (m_jwt) {
				body["jwt"] = m_jwt->raw;
			}
			if (auto res = m_cli.Post(url, body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					std::string jwt = result["jwt"].get<std::string>();
					size_t first_sep = jwt.find_first_of('.');
					size_t second_sep = jwt.find_first_of('.', first_sep + 1);
					std::string payload_b64 = jwt.substr(first_sep + 1, second_sep - first_sep);
					char* payload_ascii_bin = util::base64_decode(payload_b64.data());
					std::string payload_ascii(payload_ascii_bin);
					delete[] payload_ascii_bin;
					nlohmann::json payload_json = nlohmann::json::parse(payload_ascii);
					m_jwt = auth::jwt{
						.exp = payload_json["exp"].get<std::time_t>(),
						.username = payload_json["username"].get<std::string>(),
						.id = payload_json["id"].get<int>(),
						.confirmed = payload_json["confirmed"].get<bool>(),
						.tier = payload_json["tier"].get<int>(),
						.raw = jwt,
					};
					return {
						.success = true,
						.confirmed = m_jwt->confirmed
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
std::future<auth::reverify_response> auth::resend_verify() {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this]() -> auth::reverify_response {
		try {
			nlohmann::json body;
			if (m_jwt) {
				body["jwt"] = m_jwt->raw;
			}
			if (auto res = m_cli.Post("/resend-verify", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.msg = result["message"].get<std::string>()
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
