#include "menu_bar.hpp"
#include <cstring>

#include "context.hpp"
#include "image_text_button.hpp"

#include "api.hpp"
#include "debug.hpp"
#include "imgui.h"
#include "resource.hpp"
#include "tilemap.hpp"
#include "util.hpp"

menu_bar::menu_bar()
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
	std::memset(m_user_email, 0, 150);
	m_pword_just_reset = false;

	m_outer_player_color[0] = context::get().get_player_outline().r / 255.f;
	m_outer_player_color[1] = context::get().get_player_outline().g / 255.f;
	m_outer_player_color[2] = context::get().get_player_outline().b / 255.f;
	m_outer_player_color[3] = context::get().get_player_outline().a / 255.f;

	m_inner_player_color[0] = context::get().get_player_fill().r / 255.f;
	m_inner_player_color[1] = context::get().get_player_fill().g / 255.f;
	m_inner_player_color[2] = context::get().get_player_fill().b / 255.f;
	m_inner_player_color[3] = context::get().get_player_fill().a / 255.f;

	m_preview_player_rt.create(64, 64);
	m_preview_player.setScale(1.0, -1.0);
	m_preview_player.setPosition(0, 64);

	m_preview_player.set_animation("dash");
	m_preview_player.set_fill_color(context::get().get_player_fill());
	m_preview_player.set_outline_color(context::get().get_player_outline());
}

void menu_bar::process_event(sf::Event e) {
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
	m_submit_color_handle.poll();

	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::BeginMainMenuBar();

	// Authentication
	if (auth::get().authed() && !m_auth_unresolved()) {
		ImGui::Image(resource::get().imtex("assets/gui/large_play.png"), ImVec2(26, 26));
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
		ImGui::OpenPopup("Settings###Settings");
	}
	if (ImGui::BeginPopupModal("Settings###Settings", nullptr, modal_flags)) {
		// preview player rendering
		m_preview_player.update();
		m_preview_player_rt.clear(sf::Color(0xC8AD7FFF));
		m_preview_player_rt.draw(m_preview_player);
		m_preview_player_rt.display();

		ImGui::BeginTable("###Buttons", 2);
		for (key k = key::LEFT; k <= key::RESTART; k = key(int(k) + 1)) {
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
		ImGuiColorEditFlags ceditflags = ImGuiColorEditFlags_None;
		ImGui::Image(m_preview_player_rt.getTexture(), sf::Vector2f(64, 64));
		if (ImGui::ColorEdit4("Player Outline", m_outer_player_color, ceditflags)) {
			sf::Color oc = sf::Color(						 //
				std::floor(m_outer_player_color[0] * 255),	 //
				std::floor(m_outer_player_color[1] * 255),	 //
				std::floor(m_outer_player_color[2] * 255),	 //
				std::floor(m_outer_player_color[3] * 255));
			context::get().set_player_outline(oc);
			m_preview_player.set_outline_color(oc);
		}
		if (ImGui::ColorEdit4("Player Fill", m_inner_player_color, ceditflags)) {
			sf::Color fc = sf::Color(						 //
				std::floor(m_inner_player_color[0] * 255),	 //
				std::floor(m_inner_player_color[1] * 255),	 //
				std::floor(m_inner_player_color[2] * 255),	 //
				std::floor(m_inner_player_color[3] * 255));
			context::get().set_player_fill(fc);
			m_preview_player.set_fill_color(fc);
		}
		if (m_submit_color_handle.ready()) {
			auto res = m_submit_color_handle.get();
			if (res.success) {
				m_submit_color_handle.reset();
			} else {
				ImGui::TextColored(sf::Color::Red, "Could not upload color: %s", res.error->c_str());
			}
		}
		const std::string upload_colors_label = m_submit_color_handle.fetching() ? "Uploading...###UploadColors" : "Upload Colors###UploadColors";
		ImGui::BeginDisabled(m_submit_color_handle.fetching());
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/upload.png"), upload_colors_label.c_str())) {
			if (!m_submit_color_handle.fetching())
				m_submit_color_handle.reset(api::get().set_color(context::get().get_player_fill(), context::get().get_player_outline()));
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			if (m_submit_color_handle.fetching())
				ImGui::SetTooltip("Uploading your color to the server...");
			else
				ImGui::SetTooltip("Set this color as your publically visible one");
		}
		// alt controls toggle
		ImGui::Checkbox("BlockBros controls", &context::get().alt_controls());
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Left & right to climb ladders");
		}
		// gridlines
		ImGui::Checkbox("Editor grid lines", &context::get().grid_lines());
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Show grid lines in the editor");
		}
		// volume controls
		if (ImGui::SliderFloat("Music", &context::get().music_volume(), 0, 100, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
			resource::get().reload_music_volume();
		}
		if (ImGui::SliderFloat("SFX", &context::get().sfx_volume(), 0, 100, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
			resource::get().play_sound("gameover");
		}
		// fps limit control
		if (ImGui::SliderInt("Max FPS", &context::get().fps_limit(), 0, 360, "%d", ImGuiSliderFlags_AlwaysClamp)) {
			resource::get().window().setFramerateLimit(context::get().fps_limit());
		}
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

	if (m_self_modal) {
		m_self_modal->imdraw(sm);
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
