#include "menu_bar.hpp"
#include <cstring>

#include "image_text_button.hpp"

#include "api.hpp"
#include "debug.hpp"
#include "imgui.h"
#include "resource.hpp"
#include "tilemap.hpp"
#include "util.hpp"

namespace ImGui {

AppMenuBar::AppMenuBar()
	: m_listening_key(),
	  m_rules_gifs({ std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/run.png"), 33, { 240, 240 }, 20),
									"Use left & right to run"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/jump.png"), 19, { 240, 240 }, 20),
									"Space to jump"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/dash.png"), 19, { 240, 240 }, 20),
									"Down to dash"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/wallkick.png"), 37, { 240, 240 }, 20),
									"Left + Right to wallkick"),
					 std::make_pair(ImGui::Gif(resource::get().tex("assets/gifs/climb.png"), 25, { 240, 240 }, 20),
									"Up & Down to climb") }),
	  m_v_code(0) {
	std::memset(m_username, 0, 50);
	std::memset(m_password, 0, 50);
	std::memset(m_email, 0, 150);
}

void AppMenuBar::process_event(sf::Event e) {
	switch (e.type) {
	case sf::Event::KeyPressed:
		if (m_listening_key) {
			// check if it's already bound
			settings::get().set_key(*m_listening_key, e.key.code);
			m_listening_key = {};
		}
		break;
	default:
		break;
	}
}

void AppMenuBar::m_open_verify_popup() {
	m_v_code = 0;
	m_verify_status.reset();
	m_reverify_status.reset();
	m_signup_status.reset();
	m_login_status.reset();
	ImGui::OpenPopup("Verify###Verify");
}

void AppMenuBar::m_gui_verify_popup() {
	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
	bool dummy					 = true;
	// VERIFICATION
	if (ImGui::BeginPopupModal("Verify###Verify", &dummy, modal_flags)) {
		if (m_reverify_status) {
			if (m_reverify_status->success)
				ImGui::TextWrapped("%s", m_reverify_status->msg->c_str());
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", m_reverify_status->error->c_str());
				ImGui::PopStyleColor();
			}
		} else if (m_verify_status && !m_verify_status->success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_verify_status->error->c_str());
			ImGui::PopStyleColor();
		}
		ImGui::TextWrapped("A verification code was sent to your email.");
		ImGui::InputScalar("Code###VCODE", ImGuiDataType_S32, &m_v_code);
		ImGui::BeginDisabled(m_verify_future.valid() || m_reverify_future.valid());
		const char* submit_text = m_verify_future.valid() ? "Checking...###VerifyCheck" : "Check###VerifyCheck";
		if (ImGui::Button(submit_text)) {
			if (!m_verify_future.valid()) {
				m_verify_future = auth::get().verify(m_v_code);
				m_reverify_status.reset();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Resend Code")) {
			if (!m_reverify_future.valid()) {
				m_reverify_future = auth::get().resend_verify();
			}
		}
		ImGui::EndDisabled();
		if (m_verify_future.valid()) {
			if (util::ready(m_verify_future)) {
				m_verify_status = m_verify_future.get();
			}
		}
		if (m_verify_status && m_verify_status->success && *(m_verify_status->confirmed)) {
			m_close_auth_popup();
		}
		if (m_reverify_future.valid()) {
			if (util::ready(m_reverify_future)) {
				m_reverify_status = m_reverify_future.get();
			}
		}
		ImGui::EndPopup();
	}
}

void AppMenuBar::imdraw(std::string& info_msg) {
	const ImTextureID tiles = resource::get().imtex("assets/tiles.png");
	sf::Texture& tiles_tex	= resource::get().tex("assets/tiles.png");

	const int tile_size = 16;

	using namespace std::chrono_literals;

	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::BeginMainMenuBar();

	// Authentication
	if (auth::get().authed() && !m_auth_unresolved()) {
		ImGui::Image(resource::get().imtex("assets/gui/large_play.png"), ImVec2(26, 26));
		ImGui::TextColored(sf::Color(228, 189, 255), "Welcome, %s!", auth::get().username().c_str());
		if (ImGui::MenuItem("Logout")) {
			auth::get().logout();
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
			ImGui::BeginDisabled(m_signup_future.valid());
			if (ImGui::BeginTabItem("Login")) {
				if (m_login_status && !m_login_status->success) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
					ImGui::TextWrapped("%s", m_login_status->error->c_str());
					ImGui::PopStyleColor();
				}
				if (ImGui::InputText("Username / Email###UserEmail", m_email, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Password###Pword", m_password, 50, ImGuiInputTextFlags_Password | iflags_enter)) {
					if (!m_login_future.valid())
						m_login_future = auth::get().login(m_email, m_password);
				}
				ImGui::BeginDisabled(m_login_future.valid());
				const char* submit_text = m_login_future.valid() ? "Submitting...###LoginSubmit" : "Submit###LoginSubmit";
				if (ImGui::Button(submit_text)) {
					if (!m_login_future.valid())
						m_login_future = auth::get().login(m_email, m_password);
				}
				ImGui::EndDisabled();
				if (m_login_future.valid()) {
					if (util::ready(m_login_future)) {
						m_login_status = m_login_future.get();
					}
				}
				if (m_login_status && m_login_status->success) {
					if (*(m_login_status->confirmed)) {
						m_close_auth_popup();
					} else {
						m_open_verify_popup();
					}
				}
				m_gui_verify_popup();
				ImGui::EndTabItem();
			}
			ImGui::EndDisabled();
			// ---- sign up ---- //
			ImGui::BeginDisabled(m_login_future.valid());
			if (ImGui::BeginTabItem("Sign up")) {
				if (m_signup_status && !m_signup_status->success) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
					ImGui::TextWrapped("%s", m_signup_status->error->c_str());
					ImGui::PopStyleColor();
				}
				if (ImGui::InputText("Email###Email", m_email, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Username###Username", m_username, 150, iflags_enter)) {
					ImGui::SetKeyboardFocusHere(0);
				}
				if (ImGui::InputText("Password###Pword", m_password, 50, ImGuiInputTextFlags_Password | iflags_enter)) {
					if (!m_signup_future.valid())
						m_signup_future = auth::get().signup(m_email, m_username, m_password);
				}
				ImGui::BeginDisabled(m_signup_future.valid());
				const char* submit_text = m_signup_future.valid() ? "Submitting...###SignupSubmit" : "Submit###SignupSubmit";
				if (ImGui::Button(submit_text)) {
					if (!m_signup_future.valid())
						m_signup_future = auth::get().signup(m_email, m_username, m_password);
				}
				ImGui::EndDisabled();
				if (m_signup_future.valid()) {
					if (util::ready(m_signup_future)) {
						m_signup_status = m_signup_future.get();
					}
				}
				if (m_signup_status && m_signup_status->success) {
					if (*(m_signup_status->confirmed)) {
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
		ImGui::OpenPopup("Settings###Settings");
	}
	if (ImGui::BeginPopupModal("Settings###Settings", nullptr, modal_flags)) {
		ImGui::BeginTable("###Buttons", 2);
		for (key k = key::LEFT; k <= key::DOWN; k = key(int(k) + 1)) {
			ImGui::PushID(int(k));
			ImGui::TableNextColumn();
			ImGui::Text("%s", key_name(k));
			ImGui::TableNextColumn();
			const char* button_name = m_listening_key && *m_listening_key == k ? "Press any key..." : key_name(settings::get().get_key(k));
			if (ImGui::Button(button_name)) {
				m_listening_key = k;
			}
			ImGui::TableNextRow();
			ImGui::PopID();
		}
		ImGui::EndTable();
		// close button
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Done")) {
			m_listening_key = {};
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
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

	ImGui::Text("[ %s ]", api::get().version());

	// debug msg
	if (info_msg.length() != 0) {
		ImGui::TextColored(sf::Color(255, 120, 120, 255), "[!] %s", info_msg.c_str());
		if (ImGui::MenuItem("OK")) {
			info_msg = "";
		}
	}

	ImGui::EndMainMenuBar();
}

bool AppMenuBar::m_auth_unresolved() const {
	return m_login_future.valid() ||
		   m_signup_future.valid() ||
		   m_login_status ||
		   m_signup_status ||
		   m_verify_future.valid() ||
		   m_verify_status ||
		   m_reverify_future.valid() ||
		   m_reverify_status;
}

void AppMenuBar::m_close_auth_popup() {
	m_login_future	  = decltype(m_login_future)();
	m_signup_future	  = decltype(m_signup_future)();
	m_verify_future	  = decltype(m_verify_future)();
	m_reverify_future = decltype(m_reverify_future)();
	m_login_status.reset();
	m_signup_status.reset();
	m_verify_status.reset();
	m_reverify_status.reset();

	std::memset(m_username, 0, 50);
	std::memset(m_password, 0, 50);
	std::memset(m_email, 0, 150);

	ImGui::CloseCurrentPopup();
}
}
