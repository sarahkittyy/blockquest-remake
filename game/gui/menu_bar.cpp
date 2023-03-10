#include "menu_bar.hpp"
#include <cstring>

#include "context.hpp"
#include "image_text_button.hpp"

#include "api.hpp"
#include "debug.hpp"
#include "imgui.h"
#include "multiplayer.hpp"
#include "resource.hpp"
#include "tilemap.hpp"
#include "util.hpp"

menu_bar::menu_bar()
	: m_rules_gifs({ std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/run.png"), 33, { 240, 240 }, 20),
									"Use left & right to run"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/jump.png"), 19, { 240, 240 }, 20),
									"Space to jump"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/dash.png"), 19, { 240, 240 }, 20),
									"Down to dash"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/wallkick.png"), 37, { 240, 240 }, 20),
									"Left + Right to wallkick"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/climb.png"), 25, { 240, 240 }, 20),
									"Up & Down to climb") }),
	  m_v_code(0),
	  m_p_icon(context::get().get_player_fill(), context::get().get_player_outline()) {
	std::memset(m_username, 0, 50);
	std::memset(m_password, 0, 50);
	std::memset(m_email, 0, 150);
	std::memset(m_user_email, 0, 150);
	m_pword_just_reset = false;
}

void menu_bar::process_event(sf::Event e) {
	if (m_settings_modal) {
		m_settings_modal->process_event(e);
	}
}

void menu_bar::m_open_verify_popup() {
	m_v_code = 0;
	m_verify_handle.reset();
	m_reverify_handle.reset();
	m_signup_handle.reset();
	m_login_handle.reset();
	ImGui::OpenPopup("Verify###Verify");
}

void menu_bar::m_gui_verify_popup() {
	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
	bool dummy					 = true;
	// VERIFICATION
	if (ImGui::BeginPopupModal("Verify###Verify", &dummy, modal_flags)) {
		if (m_reverify_handle.ready()) {
			auto status = m_reverify_handle.get();
			if (status.success)
				ImGui::TextWrapped("%s", status.msg->c_str());
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", status.error->c_str());
				ImGui::PopStyleColor();
			}
		} else if (m_verify_handle.ready() && !m_verify_handle.get().success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_verify_handle.get().error->c_str());
			ImGui::PopStyleColor();
		}
		ImGui::TextWrapped("A verification code was sent to your email.");
		ImGui::InputScalar("Code###VCODE", ImGuiDataType_S32, &m_v_code);
		ImGui::BeginDisabled(m_verify_handle.fetching() || m_reverify_handle.fetching());
		const char* submit_text = m_verify_handle.fetching() ? "Checking...###VerifyCheck" : "Check###VerifyCheck";
		if (ImGui::Button(submit_text)) {
			if (!m_verify_handle.fetching()) {
				m_verify_handle.reset(auth::get().verify(m_v_code));
				m_reverify_handle.reset();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Resend Code")) {
			if (!m_reverify_handle.fetching()) {
				m_reverify_handle.reset(auth::get().resend_verify());
			}
		}
		ImGui::EndDisabled();
		if (m_verify_handle.ready() && m_verify_handle.get().success && m_verify_handle.get().confirmed) {
			m_close_auth_popup();
		}
		ImGui::EndPopup();
	}
}

void menu_bar::imdraw(std::string& info_msg, fsm* sm) {
	const ImTextureID tiles = resource::get().imtex("assets/tiles.png");
	sf::Texture& tiles_tex	= resource::get().tex("assets/tiles.png");

	const int tile_size = 64;

	using namespace std::chrono_literals;

	m_login_handle.poll();
	m_signup_handle.poll();
	m_verify_handle.poll();
	m_reverify_handle.poll();

	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::BeginMainMenuBar();

	// Authentication
	if (auth::get().authed() && !m_auth_unresolved()) {
		ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(m_p_icon.get_tex().getNativeHandle()),
											 ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 5),
											 ImVec2(ImGui::GetCursorScreenPos().x + 16, ImGui::GetCursorScreenPos().y + 16 + 5));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 21);
		ImGui::TextColored(sf::Color(228, 189, 255), "Welcome, %s!", auth::get().username().c_str());
		if (ImGui::MenuItem("Profile")) {
			m_self_modal.reset(new user_modal(auth::get().get_jwt().id));
			m_self_modal->open();
		}
		if (ImGui::MenuItem("Logout")) {
			auth::get().logout();
			m_self_modal.reset();
		}
	} else {
		// login menu
		if (ImGui::MenuItem("Login")) {
			ImGui::OpenPopup("Authentication###Authentication");
		}
		bool auth_popup_opened = true;
		if (ImGui::BeginPopupModal("Authentication###Authentication", &auth_popup_opened, modal_flags)) {
			ImGui::BeginTabBar("###AuthOptions");
			ImGuiInputTextFlags iflags_none	 = ImGuiInputTextFlags_None;
			ImGuiInputTextFlags iflags_enter = ImGuiInputTextFlags_EnterReturnsTrue;
			// ---- login ---- //
			ImGui::BeginDisabled(m_signup_handle.fetching());
			if (ImGui::BeginTabItem("Login")) {
				if (m_login_handle.ready() && !m_login_handle.get().success) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
					ImGui::TextWrapped("%s", m_login_handle.get().error->c_str());
					ImGui::PopStyleColor();
				}
				if (m_pword_just_reset || (m_fgp_modal && m_fgp_modal->success())) {
					m_fgp_modal.reset();
					m_pword_just_reset = true;
					ImGui::TextColored(sf::Color::Green, "Password was just reset.");
				}
				if (ImGui::InputText("Username / Email###UserEmailmbar", m_user_email, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Password###Pword", m_password, 50, ImGuiInputTextFlags_Password | iflags_enter)) {
					if (!m_login_handle.fetching())
						m_login_handle.reset(auth::get().login(m_user_email, m_password));
				}
				ImGui::BeginDisabled(m_login_handle.fetching());
				const char* submit_text = m_login_handle.fetching() ? "Submitting...###LoginSubmit" : "Submit###LoginSubmit";
				if (ImGui::Button(submit_text)) {
					if (!m_login_handle.fetching())
						m_login_handle.reset(auth::get().login(m_user_email, m_password));
				}
				ImGui::SameLine();
				if (ImGui::Button("Forgot Password###FGPWORD")) {
					m_fgp_modal.reset(new forgot_password_modal());
					m_fgp_modal->open();
				}
				ImGui::EndDisabled();
				if (m_login_handle.ready() && m_login_handle.get().success) {
					if (m_login_handle.get().confirmed.value_or(false)) {
						m_close_auth_popup();
					} else {
						m_open_verify_popup();
					}
				}
				m_gui_verify_popup();
				if (m_fgp_modal) {
					m_fgp_modal->imdraw();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndDisabled();
			// ---- sign up ---- //
			ImGui::BeginDisabled(m_login_handle.fetching());
			if (ImGui::BeginTabItem("Sign up")) {
				if (m_signup_handle.ready() && !m_signup_handle.get().success) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
					ImGui::TextWrapped("%s", m_signup_handle.get().error->c_str());
					ImGui::PopStyleColor();
				}
				if (ImGui::InputText("Email###Email", m_email, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Username###Username", m_username, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Password###Pword", m_password, 50, ImGuiInputTextFlags_Password | iflags_enter)) {
					if (!m_signup_handle.fetching())
						m_signup_handle.reset(auth::get().signup(m_email, m_username, m_password));
				}
				ImGui::BeginDisabled(m_signup_handle.fetching());
				const char* submit_text = m_signup_handle.fetching() ? "Submitting...###SignupSubmit" : "Submit###SignupSubmit";
				if (ImGui::Button(submit_text)) {
					if (!m_signup_handle.fetching())
						m_signup_handle.reset(auth::get().signup(m_email, m_username, m_password));
				}
				ImGui::EndDisabled();
				if (m_signup_handle.ready() && m_signup_handle.get().success) {
					if (m_signup_handle.get().confirmed.value_or(false)) {
						m_close_auth_popup();
					} else {
						m_open_verify_popup();
					}
				}
				m_gui_verify_popup();
				ImGui::EndTabItem();
			}
			ImGui::EndDisabled();

			ImGui::EndTabBar();
			ImGui::EndPopup();
		}
	}

	// settings
	if (ImGui::MenuItem("Settings")) {
		m_settings_modal.reset(new settings_modal());
		m_settings_modal->open();
	}
	// rules
	if (ImGui::MenuItem("Rules")) {
		ImGui::OpenPopup("Rules###Rules");
	}
	if (ImGui::BeginPopup("Rules###Rules")) {
		ImGui::BeginTable("###Rules", 2);
		ImGui::TableNextColumn();
		ImRect uvs;

		uvs = tilemap::calc_uvs(tile::begin, tiles_tex, tile_size);
		ImGui::Text("Start here");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = tilemap::calc_uvs(tile::end, tiles_tex, tile_size);
		ImGui::TableNextColumn();
		ImGui::Text("Try to reach the goal");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = tilemap::calc_uvs(tile::spike, tiles_tex, tile_size);
		ImGui::TableNextColumn();
		ImGui::Text("Avoid spikes");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = tilemap::calc_uvs(tile::gravity, tiles_tex, tile_size);
		ImGui::TableNextColumn();
		ImGui::Text("Flip gravity");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = tilemap::calc_uvs(tile::ladder, tiles_tex, tile_size);
		ImGui::TableNextColumn();
		ImGui::Text("Climb up ladders");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		uvs = tilemap::calc_uvs(tile::ice, tiles_tex, tile_size);
		ImGui::TableNextColumn();
		ImGui::Text("Watch your step");
		ImGui::TableNextColumn();
		ImGui::Image(
			tiles,
			ImVec2(64, 64),
			uvs.Min, uvs.Max);
		ImGui::TableNextRow();

		ImGui::EndTable();
		ImGui::EndPopup();
	}
	// controls
	if (ImGui::MenuItem("Controls")) {
		ImGui::OpenPopup("Controls###Controls");
	}
	if (ImGui::BeginPopup("Controls###Controls")) {
		ImGui::BeginTable("###Gifs", 5);
		for (auto& [gif, desc] : m_rules_gifs) {
			ImGui::TableNextColumn();
			gif.update();
			gif.draw({ 240, 240 });
			ImGui::Text("%s", desc);
		}
		ImGui::EndTable();
		ImGui::EndPopup();
	}

	if (auth::get().authed() && !m_auth_unresolved()) {
		int istate = static_cast<int>(multiplayer::get().get_state());
		int x	   = istate % 2;
		int y	   = istate / 2;
		ImVec2 uv0(x / 2.f, y / 2.f);
		ImVec2 uv1(x / 2.f + 0.5f, y / 2.f + 0.5f);
		if (ImGui::MenuItem("Multiplayer")) {
			switch (multiplayer::get().get_state()) {
			case multiplayer::state::CONNECTED:
				multiplayer::get().disconnect();
				break;
			case multiplayer::state::CONNECTING:
				break;
			case multiplayer::state::DISCONNECTED:
				multiplayer::get().connect();
				break;
			}
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			switch (multiplayer::get().get_state()) {
			case multiplayer::state::CONNECTED:
				ImGui::Text("Connected. Click to disconnect.");
				break;
			case multiplayer::state::CONNECTING:
				ImGui::Text("Connecting...");
				break;
			case multiplayer::state::DISCONNECTED:
				ImGui::Text("Disconnected. Click to connect.");
				break;
			}
			ImGui::EndTooltip();
		}
		ImGui::Image(resource::get().imtex("assets/gui/connection.png"), ImVec2(26, 26), uv0, uv1);
	}

	ImGui::Text("[ %s ]", api::get().version());

	// debug msg
	if (info_msg.length() != 0) {
		ImGui::TextColored(sf::Color(255, 120, 120, 255), "[!] %s", info_msg.c_str());
		if (ImGui::MenuItem("OK")) {
			info_msg = "";
		}
	}

	if (m_self_modal) {
		m_self_modal->imdraw(sm);
	}
	if (m_settings_modal) {
		bool update_icon = false;
		m_settings_modal->imdraw(&update_icon);
		if (update_icon) {
			m_p_icon.set_fill_color(context::get().get_player_fill());
			m_p_icon.set_outline_color(context::get().get_player_outline());
		}
	}

	ImGui::EndMainMenuBar();
}

bool menu_bar::m_auth_unresolved() const {
	return m_login_handle.fetching() ||
		   m_signup_handle.fetching() ||
		   m_login_handle.ready() ||
		   m_signup_handle.ready() ||
		   m_verify_handle.fetching() ||
		   m_verify_handle.ready() ||
		   m_reverify_handle.fetching() ||
		   m_reverify_handle.ready();
}

void menu_bar::m_close_auth_popup() {
	m_login_handle.reset();
	m_signup_handle.reset();
	m_verify_handle.reset();
	m_reverify_handle.reset();

	std::memset(m_username, 0, 50);
	std::memset(m_password, 0, 50);
	std::memset(m_email, 0, 150);
	std::memset(m_user_email, 0, 150);

	ImGui::CloseCurrentPopup();
}
