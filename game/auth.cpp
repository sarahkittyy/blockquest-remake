#include "auth.hpp"

#include "api.hpp"
#include "debug.hpp"
#include "settings.hpp"
#include "util.hpp"

auth::auth()
	: m_cli(settings::get().server_url()) {
#ifdef NO_VERIFY_CERTS
	m_cli.enable_server_certificate_verification(false);
#endif
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

int auth::id() const {
	return authed() ? m_jwt->id : -1;
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
					std::string payload_ascii;
					payload_ascii.resize(500);
					util::base64_decode(payload_b64, payload_ascii.data(), 500);
					nlohmann::json payload_json = nlohmann::json::parse(payload_ascii);
					m_jwt = auth::jwt{
						.exp = payload_json["exp"].get<std::time_t>(),
						.username = payload_json["username"].get<std::string>(),
						.id = payload_json["id"].get<int>(),
						.confirmed = payload_json["confirmed"].get<bool>(),
						.tier = payload_json["tier"].get<int>(),
						.raw = jwt,
					};
					api::get().flush_colors();
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
		} catch (nlohmann::json::parse_error& e) {
			debug::log() << e.what() << "\n";
			return {
				.success = false,
				.error ="Invalid server response (contact the developer!)"
			};
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
					std::string payload_b64 = jwt.substr(first_sep + 1, second_sep - first_sep - 1);
					std::string payload_ascii;
					payload_ascii.resize(500);
					util::base64_decode(payload_b64, payload_ascii.data(), 500);
					nlohmann::json payload_json = nlohmann::json::parse(payload_ascii);
					m_jwt = auth::jwt{
						.exp = payload_json["exp"].get<std::time_t>(),
						.username = payload_json["username"].get<std::string>(),
						.id = payload_json["id"].get<int>(),
						.confirmed = payload_json["confirmed"].get<bool>(),
						.tier = payload_json["tier"].get<int>(),
						.raw = jwt,
					};
					api::get().flush_colors();
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
		} catch (nlohmann::json::parse_error& e) {
			debug::log() << e.what() << "\n";
			return {
				.success = false,
				.error ="Invalid server response (contact the developer!)"
			};
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

std::future<auth::forgot_password_response> auth::forgot_password(std::string email) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, email]() -> auth::forgot_password_response {
		try {
			nlohmann::json body;
			body["email"] = email;
			if (auto res = m_cli.Post("/forgot-password", body.dump(), "application/json")) {
				nlohmann::json result = nlohmann::json::parse(res->body);
				if (res->status == 200) {
					return {
						.success = true,
						.exp = result["exp"].get<std::time_t>(),
						.exists = result["exists"].get<bool>(),
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

std::future<auth::reset_password_response> auth::reset_password(std::string email, std::string code, std::string newPassword) {
	// clang-format off
	using namespace std::chrono_literals;
	return std::async([this, email, code, newPassword]() -> auth::reset_password_response {
		try {
			nlohmann::json body;
			body["email"] = email;
			body["code"] = code;
			body["password"] = newPassword;
			if (auto res = m_cli.Post("/reset-password", body.dump(), "application/json")) {
				if (res->status == 200) {
					return {
						.success = true,
					};
				} else {
					nlohmann::json result = nlohmann::json::parse(res->body);
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
					std::string payload_b64 = jwt.substr(first_sep + 1, second_sep - first_sep - 1);
					std::string payload_ascii;
					payload_ascii.resize(500);
					util::base64_decode(payload_b64, payload_ascii.data(), 500);
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
