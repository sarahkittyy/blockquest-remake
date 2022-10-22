#include "main.hpp"

#include <imgui-SFML.h>
#include <imgui.h>

#include "../gui/image_text_button.hpp"

#include "edit.hpp"

namespace states {

main::main(resource& r)
	: state(r),
	  m_logo_new(r.tex("assets/logo_new.png")),
	  m_play(r.tex("assets/gui/play.png")),
	  m_create(r.tex("assets/gui/create.png")) {
}

main::~main() {
}

void main::update(fsm* sm, sf::Time dt) {
}

void main::imdraw(fsm* sm) {
	// clang-format off
	ImGui::Begin("BlockQuest Remake", nullptr); {
		sf::Vector2f win_sz = ImGui::GetWindowSize();
		ImGui::Image(m_logo_new);

		ImGui::NewLine();

		if (ImGui::ImageButtonWithText((ImTextureID)m_create.getNativeHandle(), "Create")) {
			sm->swap_state<edit>();
		}
	} ImGui::End();
	// clang-format on
}

void main::draw(sf::RenderTarget& t, sf::RenderStates s) const {
	s.transform *= getTransform();
}

}
