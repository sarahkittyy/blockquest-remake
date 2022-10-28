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
	bool authed() const;

	struct response {
		bool success;
		std::optional<std::string> error;
	};

	struct jwt {
		std::time_t exp;
		std::string username;
		int tier;
		std::string raw;
	};

	std::future<response> signup(std::string email, std::string username, std::string password);
	std::future<response> login(std::string email_or_username, std::string password);
	void logout();

	std::string username() const;	// the current authed username
	int tier() const;				// the user's tier that we're logged in as

private:
	auth();
	auth(const auth& other) = delete;
	auth(auth&& other)		= delete;

	httplib::Client m_cli;

	std::optional<jwt> m_jwt;	// optional jwt we receive from logging in
};
