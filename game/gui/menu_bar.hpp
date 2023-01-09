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

#include "api_handle.hpp"

#include "gui/forgot_password_modal.hpp"
#include "gui/user_modal.hpp"

class menu_bar {
public:
	menu_bar();

	void imdraw(std::string& info_msg, fsm* sm);
	void process_event(sf::Event e);

private:
	std::optional<key> m_listening_key;	  // key to listen to for changing controls

	std::array<std::pair<ImGui::Gif, const char*>, 5> m_rules_gifs;	  // the 5 gifs of the rules

	// buffers for auth info
	char m_username[50];
	char m_password[50];
	char m_email[150];
	char m_user_email[150];
	int m_v_code;

	api_handle<auth::response> m_login_handle;
	api_handle<auth::response> m_signup_handle;
	api_handle<auth::response> m_verify_handle;
	api_handle<auth::reverify_response> m_reverify_handle;

	std::unique_ptr<user_modal> m_self_modal;

	std::unique_ptr<forgot_password_modal> m_fgp_modal;
	bool m_pword_just_reset;

	bool m_auth_unresolved() const;	  // are the login/signup futures / statues all reset properly?
	void m_open_verify_popup();
	void m_gui_verify_popup();

	void m_close_auth_popup();	 // close the auth popup and reset all values
};
