#include "level_card.hpp"

#include "debug.hpp"
#include "gui/image_text_button.hpp"
#include "imgui.h"
#include "resource.hpp"

#include "auth.hpp"
#include "context.hpp"
#include "fsm.hpp"
#include "gui/user_modal.hpp"
#include "states/edit.hpp"
#include "util.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

int level_card::m_next_id = 0;

level_card::level_card(api::level& lvl, sf::Color bg)
	: m_bg(bg),
	  m_lvl(lvl),
	  m_tmap(resource::get().tex("assets/tiles.png"), 32, 32, 16),
	  m_lb_modal(lvl),
	  m_comment_modal(lvl) {

	m_map_tex.create(256, 256);
	m_tmap.load(m_lvl.code);
	m_map_tex.setView(sf::View(sf::FloatRect(0, 0, 512, 512)));
	m_map_tex.setSmooth(true);
	m_map_tex.clear(m_bg);
	m_map_tex.draw(m_tmap);
	m_map_tex.display();

	m_ex_id = m_next_id++;
}

void level_card::imdraw(fsm* sm) {
	// title / auth
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(0xFB8CABFF), "%s (v%d)", m_lvl.title.c_str(), m_lvl.version);
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(0xFFFFFFFF), "(%s)", m_lvl.author.c_str());
	ImGui::SameLine();
	ImGui::Image(resource::get().imtex("assets/gui/play.png"), ImVec2(16, 16));
	ImGui::EndGroup();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("View player profile");
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		if (ImGui::IsMouseClicked(0)) {
			m_user_modal.reset(new user_modal(m_lvl.author));
			m_user_modal->open();
		}
	}
	ImGui::SameLine();
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(0xFB8CABFF), "#%d", m_lvl.id);
	ImVec2 ip = ImGui::GetCursorScreenPos();
	ImGui::Image(m_map_tex);
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::GetWindowDrawList()->AddImage(resource::get().imtex("assets/gui/download_large.png"), ip, ImVec2(ip.x + 256, ip.y + 256));
		if (ImGui::IsMouseClicked(0)) {
			api::get().ping_download(m_lvl.id);
			sm->swap_state<states::edit>(m_lvl);
		}
	}

	// leaderboard btn
	std::ostringstream leaderboard_label;
	if (m_lvl.record) {
		leaderboard_label << "WR: " << std::fixed << std::setprecision(2)
						  << m_lvl.record->time << "s by " << m_lvl.record->user;
	} else {
		leaderboard_label << "No WR yet";
	}
	leaderboard_label << "###Leaderboard" << m_ex_id;
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/trophy.png"), leaderboard_label.str().c_str())) {
		m_lb_modal.open();
	}

	// likes/dislikes/downloads/records
	ImVec2 x16(16, 16);
	ImVec2 uv0(0, 0), uv1(1, 1);
	int fp				  = 4;
	ImVec4 bg			  = ImVec4(1, 0, 0, 0);
	std::string likes	  = std::to_string(m_lvl.likes) + "###LIKES" + std::to_string(m_ex_id);
	std::string dislikes  = std::to_string(m_lvl.dislikes) + "###DISLIKES" + std::to_string(m_ex_id);
	std::string downloads = std::to_string(m_lvl.downloads) + "###DOWNLOADS" + std::to_string(m_ex_id);
	std::string comments  = std::to_string(m_lvl.comments) + "###COMMENTS" + std::to_string(m_ex_id);
	std::string records	  = std::to_string(m_lvl.records) + "###RECORDS" + std::to_string(m_ex_id);
	ImU32 likes_tcol	  = ImGui::GetColorU32(sf::Color::Green);
	ImU32 dislikes_tcol	  = ImGui::GetColorU32(sf::Color::Red);
	bool authed			  = auth::get().authed();
	ImGui::PushStyleColor(ImGuiCol_Text, likes_tcol);
	ImGui::BeginDisabled(m_vote_handle.fetching() || m_lvl.myVote.value_or(0) == 1 || !authed);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/heart.png"), likes.c_str(), x16, uv0, uv1, fp, bg)) {
		if (!m_vote_handle.fetching())
			m_vote_handle.reset(api::get().vote_level(m_lvl, api::vote::LIKE));
	}
	ImGui::EndDisabled();
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip(m_lvl.myVote.value_or(0) == 1 ? "Liked!" : "Like");
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Text, dislikes_tcol);
	ImGui::BeginDisabled(m_vote_handle.fetching() || m_lvl.myVote.value_or(0) == -1 || !authed);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/spike.png"), dislikes.c_str(), x16, uv0, uv1, fp, bg)) {
		if (!m_vote_handle.fetching())
			m_vote_handle.reset(api::get().vote_level(m_lvl, api::vote::DISLIKE));
	}
	ImGui::EndDisabled();
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip(m_lvl.myVote.value_or(0) == -1 ? "Disliked!" : "Dislike");
	}

	ImGui::SameLine();

	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), downloads.c_str(), x16, uv0, uv1, fp, bg)) {
		api::get().ping_download(m_lvl.id);
		sm->swap_state<states::edit>(m_lvl);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Total downloads");
	}

	ImGui::SameLine();

	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/trophy.png"), records.c_str(), x16, uv0, uv1, fp, bg)) {
		m_lb_modal.open();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Scores posted");
	}

	ImGui::SameLine();

	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/comment.png"), comments.c_str(), x16, uv0, uv1, fp, bg)) {
		m_comment_modal.open();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Comments");
	}

	m_vote_handle.poll();
	if (m_vote_handle.ready()) {
		auto status = m_vote_handle.get();
		if (status.success) {
			m_lvl.likes	   = status.level->likes;
			m_lvl.dislikes = status.level->dislikes;
			m_lvl.myVote   = status.level->myVote;
			m_vote_handle.reset();
		}
	}

	m_lb_modal.imdraw(sm);
	m_comment_modal.imdraw(sm);
	if (m_user_modal) m_user_modal->imdraw(sm);

	// extra info

	char date_fmt[100];
	tm* date_tm = std::localtime(&m_lvl.createdAt);
	std::strftime(date_fmt, 100, "%D %r", date_tm);
	ImGui::Text("Created: %s", date_fmt);
	if (m_lvl.createdAt != m_lvl.updatedAt) {
		date_tm = std::localtime(&m_lvl.updatedAt);
		std::strftime(date_fmt, 100, "%D %r", date_tm);
		ImGui::TextWrapped("Updated: %s", date_fmt);
	}
	ImGui::PushStyleColor(ImGuiCol_Text, 0x48c8f0FF);
	ImGui::TextWrapped("%s", m_lvl.description.c_str());
	ImGui::PopStyleColor();
}
