#pragma once

#include <ctime>
#include <future>
#include <optional>
#include <string>

#include "httplib.h"
#include "json.hpp"

// singleton class that manages the authentication state of the user
class auth {
public:
	static auth& get();

	// is the user logged in
	bool authed(bool confirmed = true) const;

	struct response {
		bool success;
		std::optional<bool> confirmed;
		std::optional<std::string> error;
	};

	struct jwt {
		std::time_t exp;
		std::string username;
		int id;
		bool confirmed;
		int tier;
		std::string raw;
	};

	struct reverify_response {
		bool success;
		std::optional<std::string> msg;
		std::optional<std::string> error;
	};

	struct forgot_password_response {
		bool success;
		std::optional<std::time_t> exp;
		std::optional<bool> exists;
		std::optional<std::string> error;
	};

	struct reset_password_response {
		bool success;
		std::optional<std::string> error;
	};

	std::future<response> signup(std::string email, std::string username, std::string password);
	std::future<response> login(std::string email_or_username, std::string password);
	void logout();

	std::future<forgot_password_response> forgot_password(std::string email);
	std::future<reset_password_response> reset_password(std::string email, std::string code, std::string newPassword);

	std::future<response> verify(int code);
	std::future<reverify_response> resend_verify();

	std::string username() const;	// the current authed username
	int tier() const;				// the user's tier that we're logged in as

	jwt get_jwt();

	void add_jwt_to_body(nlohmann::json& body);	  // adds the jwt to the body of a query if it exists

private:
	auth();
	auth(const auth& other) = delete;
	auth(auth&& other)		= delete;

	httplib::Client m_cli;

	std::optional<jwt> m_jwt;	// optional jwt we receive from logging in
};
