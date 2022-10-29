#include "search.hpp"

#include "imgui-SFML.h"
#include "imgui.h"

#include "../gui/image_text_button.hpp"

#include "edit.hpp"

namespace states {

search::search()
	: m_logo_new(resource::get().tex("assets/logo_new.png")),
	  m_play(resource::get().tex("assets/gui/play.png")) {
}

search::~search() {
}

void search::update(fsm* sm, sf::Time dt) {
}

void search::imdraw(fsm* sm) {
	// clang-format off
	ImGui::Begin("BlockQuest Remake", nullptr); {
		sf::Vector2f win_sz = ImGui::GetWindowSize();
		ImGui::Image(m_logo_new);

		ImGui::NewLine();

		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/create.png"), "Create")) {
			sm->swap_state<edit>();
		}
	} ImGui::End();
	// clang-format on
}

void search::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
}

}
