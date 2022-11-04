#include "level_card.hpp"

#include "debug.hpp"
#include "gui/image_text_button.hpp"
#include "imgui.h"
#include "resource.hpp"

#include "auth.hpp"
#include "util.hpp"

namespace ImGui {

ApiLevelTile::ApiLevelTile(api::level& lvl, sf::Color bg)
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
	// title / auth
	ImGui::PushStyleColor(ImGuiCol_Text, 0xFB8CABFF);
	ImGui::Text("%s (%s) #%d", m_lvl.title.c_str(), m_lvl.author.c_str(), m_lvl.id);
	ImGui::PopStyleColor();
	ImGui::Image(m_map_tex);

	// download btn
	std::string download_label = "Download (" + std::to_string(m_lvl.downloads) + ")";
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), download_label.c_str())) {
		if (download) {
			*download = true;
		}
	}
	// likes/dislikes
	ImVec2 x16(16, 16);
	ImVec2 uv0(0, 0), uv1(1, 1);
	int fp				 = 4;
	ImVec4 bg			 = ImVec4(1, 0, 0, 0);
	std::string likes	 = std::to_string(m_lvl.likes) + "###LIKES";
	std::string dislikes = std::to_string(m_lvl.dislikes) + "###DISLIKES";
	ImU32 likes_tcol	 = ImGui::GetColorU32(sf::Color::Green);
	ImU32 dislikes_tcol	 = ImGui::GetColorU32(sf::Color::Red);
	bool authed			 = auth::get().authed();
	ImGui::PushStyleColor(ImGuiCol_Text, likes_tcol);
	ImGui::BeginDisabled(m_vote_future.valid() || m_lvl.myVote.value_or(0) == 1 || !authed);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/heart.png"), likes.c_str(), x16, uv0, uv1, fp, bg)) {
		if (!m_vote_future.valid())
			m_vote_future = api::get().vote_level(m_lvl, api::vote::LIKE);
	}
	ImGui::EndDisabled();
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip(m_lvl.myVote.value_or(0) == 1 ? "Liked!" : "Like");
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, dislikes_tcol);
	ImGui::BeginDisabled(m_vote_future.valid() || m_lvl.myVote.value_or(0) == -1 || !authed);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/spike.png"), dislikes.c_str(), x16, uv0, uv1, fp, bg)) {
		if (!m_vote_future.valid())
			m_vote_future = api::get().vote_level(m_lvl, api::vote::DISLIKE);
	}
	ImGui::EndDisabled();
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip(m_lvl.myVote.value_or(0) == -1 ? "Disliked!" : "Dislike");
	}

	if (m_vote_future.valid() && util::ready(m_vote_future)) {
		m_vote_status = m_vote_future.get();
		if (m_vote_status->success) {
			m_lvl.likes	   = m_vote_status->level->likes;
			m_lvl.dislikes = m_vote_status->level->dislikes;
			m_lvl.myVote   = m_vote_status->level->myVote;
		}
	}

	// extra info

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
