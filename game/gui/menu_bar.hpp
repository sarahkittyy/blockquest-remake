#pragma once

#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <array>
#include <future>
#include <optional>

#include "gif.hpp"

#include "auth.hpp"
#include "settings.hpp"

namespace ImGui {

class AppMenuBar {
public:
	AppMenuBar();

	void imdraw(std::string& info_msg);
	void process_event(sf::Event e);

private:
	std::optional<key> m_listening_key;	  // key to listen to for changing controls

	std::array<std::pair<ImGui::Gif, const char*>, 5> m_rules_gifs;	  // the 5 gifs of the rules

	// buffers for auth info
	char m_username[50];
	char m_password[50];
	char m_email[150];
	int m_v_code;

	std::future<auth::response> m_login_future;
	std::future<auth::response> m_signup_future;
	std::future<auth::response> m_verify_future;
	std::future<auth::reverify_response> m_reverify_future;

	std::optional<auth::response> m_login_status;
	std::optional<auth::response> m_signup_status;
	std::optional<auth::response> m_verify_status;
	std::optional<auth::reverify_response> m_reverify_status;

	bool m_auth_unresolved() const;	  // are the login/signup futures / statues all reset properly?
	void m_open_verify_popup();
	void m_gui_verify_popup();

	void m_close_auth_popup();	 // close the auth popup and reset all values
};

}
