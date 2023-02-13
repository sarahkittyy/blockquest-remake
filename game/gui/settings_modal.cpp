#include "settings_modal.hpp"

#include <imgui-SFML.h>
#include <imgui.h>

#include "auth.hpp"
#include "context.hpp"
#include "gui/image_text_button.hpp"
#include "resource.hpp"

int settings_modal::m_next_id = 0;

settings_modal::settings_modal()
	: m_listening_key(),
	  m_open(false),
	  m_ex_id(m_next_id++),
	  m_modal_name("Settings###Settings" + std::to_string(m_ex_id)) {
	m_ex_id					= m_next_id++;
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

void settings_modal::open() {
	m_open = true;
	ImGui::OpenPopup(m_modal_name.c_str());
}

void settings_modal::process_event(sf::Event e) {
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

void settings_modal::imdraw() {
	ImGuiWindowFlags modal_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	m_submit_color_handle.poll();

	if (ImGui::BeginPopupModal(m_modal_name.c_str(), &m_open, modal_flags)) {
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
		bool can_upload_color				  = !m_submit_color_handle.fetching() && auth::get().authed();
		ImGui::BeginDisabled(!can_upload_color);
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/upload.png"), upload_colors_label.c_str())) {
			if (!m_submit_color_handle.fetching())
				m_submit_color_handle.reset(api::get().set_color(context::get().get_player_fill(), context::get().get_player_outline()));
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			if (m_submit_color_handle.fetching())
				ImGui::SetTooltip("Uploading your color to the server...");
			else if (!auth::get().authed())
				ImGui::SetTooltip("Not logged in!");
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
}
