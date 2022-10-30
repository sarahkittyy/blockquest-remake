#include "level_card.hpp"

#include "gui/image_text_button.hpp"
#include "resource.hpp"

namespace ImGui {

ApiLevelTile::ApiLevelTile(api::level lvl, sf::Color bg)
	: m_bg(bg),
	  m_lvl(lvl),
	  m_tmap(resource::get().tex("assets/tiles.png"), 32, 32, 16) {
	m_map_tex.create(256, 256);
	m_tmap.load(m_lvl.code);
	m_map_tex.setView(sf::View(sf::FloatRect(0, 0, 512, 512)));
	m_map_tex.setSmooth(true);
	m_map_tex.clear(m_bg);
	m_map_tex.draw(m_tmap);
	m_map_tex.display();
}

void ApiLevelTile::imdraw(bool* download) {
	if (download) {
		*download = false;
	}
	ImGui::PushStyleColor(ImGuiCol_Text, 0xFB8CABFF);
	ImGui::Text("%s (%s) #%d", m_lvl.title.c_str(), m_lvl.author.c_str(), m_lvl.id);
	ImGui::PopStyleColor();
	ImGui::Image(m_map_tex);
	std::string download_label = "Download (" + std::to_string(m_lvl.downloads) + ")";
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), download_label.c_str())) {
		if (download) {
			*download = true;
		}
	}
	char date_fmt[100];
	tm* date_tm = std::localtime(&m_lvl.createdAt);
	std::strftime(date_fmt, 100, "%D %r", date_tm);
	ImGui::TextWrapped("Created: %s", date_fmt);
	if (m_lvl.createdAt != m_lvl.updatedAt) {
		date_tm = std::localtime(&m_lvl.updatedAt);
		std::strftime(date_fmt, 100, "%D %r", date_tm);
		ImGui::TextWrapped("Updated: %s", date_fmt);
	}
	ImGui::PushStyleColor(ImGuiCol_Text, 0x48c8f0FF);
	ImGui::TextWrapped("%s", m_lvl.description.c_str());
	ImGui::PopStyleColor();
}

}
