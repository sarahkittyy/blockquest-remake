#include "level_card.hpp"

#include "debug.hpp"
#include "gui/image_text_button.hpp"
#include "imgui.h"
#include "resource.hpp"

#include "auth.hpp"
#include "context.hpp"
#include "fsm.hpp"
#include "states/edit.hpp"
#include "util.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ImGui {

ApiLevelTile::ApiLevelTile(api::level& lvl, sf::Color bg)
	: m_bg(bg),
	  m_lvl(lvl),
	  m_tmap(resource::get().tex("assets/tiles.png"), 32, 32, 16),
	  m_sort_opts{ "time", "author", "createdAt", "updatedAt" },
	  m_order_opts{ "asc", "desc" } {
	auto replay_query = context::get().replay_search_query();
	auto sort_it	  = std::find_if(std::begin(m_sort_opts), std::end(m_sort_opts),
									 [this, replay_query](const char* str) { return std::string(str) == replay_query.sortBy; });
	m_sort_selection  = sort_it != std::end(m_sort_opts) ? std::distance(std::begin(m_sort_opts), sort_it) : 0;
	auto order_it	  = std::find_if(std::begin(m_order_opts), std::end(m_order_opts),
									 [this, replay_query](const char* str) { return std::string(str) == replay_query.order; });
	m_order_selection = sort_it != std::end(m_order_opts) ? std::distance(std::begin(m_order_opts), order_it) : 0;
	m_map_tex.create(256, 256);
	m_tmap.load(m_lvl.code);
	m_map_tex.setView(sf::View(sf::FloatRect(0, 0, 512, 512)));
	m_map_tex.setSmooth(true);
	m_map_tex.clear(m_bg);
	m_map_tex.draw(m_tmap);
	m_map_tex.display();
}

void ApiLevelTile::m_gui_leaderboard_popup(fsm* sm) {
	bool dummy				= true;
	std::string modal_title = m_lvl.title + " leaderboard###Leaderboard";
	ImGuiWindowFlags flags	= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopupModal(modal_title.c_str(), &dummy, flags)) {
		// query opts
		ImGui::BeginDisabled(m_lb_handle.fetching());
		auto& replay_query = context::get().replay_search_query();
		ImGui::SetNextItemWidth(160);
		if (ImGui::Combo("Sort By###SortBy", &m_sort_selection, m_sort_opts, sizeof(m_sort_opts) / sizeof(m_sort_opts[0]))) {
			m_update_query();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140);
		if (ImGui::Combo("Order By###OrderBy", &m_order_selection, m_order_opts, sizeof(m_order_opts) / sizeof(m_order_opts[0]))) {
			m_update_query();
		}
		ImGui::SameLine();
		ImGui::SliderInt("Limit###Limit", &replay_query.limit, 1, 20, "%d", ImGuiSliderFlags_NoInput);
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Refresh")) {
			m_update_query();
		}
		ImGui::EndDisabled();
		ImGui::Separator();
		if (m_lb_handle.ready()) {
			api::replay_search_response res = m_lb_handle.get();
			// pagination buttons
			int page = m_cpage();
			ImGui::BeginDisabled(page == 0);
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Back")) {
				m_prev_page();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(m_last_page());
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/forward.png"), "Next")) {
				m_next_page();
			}
			ImGui::EndDisabled();
			// results
			if (!res.success) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", res.error->c_str());
				ImGui::PopStyleColor();
			} else {
				if (ImGui::BeginTable("###Scores", 5, ImGuiTableFlags_Borders)) {
					ImGui::TableNextRow();
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color(0xC8AD7FFF)));
					ImGui::TableNextColumn();
					ImGui::Text("Runner");
					ImGui::TableNextColumn();
					ImGui::Text("Time");
					ImGui::TableNextColumn();
					ImGui::Text("Submitted");
					ImGui::TableNextColumn();
					ImGui::Text("Version");
					ImGui::TableNextColumn();
					ImGui::Text("Replay");
					ImGui::PopStyleColor();
					for (int i = 0; i < res.scores.size(); ++i) {
						ImGui::PushID(i);
						ImGui::TableNextRow();
						auto& score = res.scores[i];
						ImGui::TableNextColumn();
						ImGui::Text("%s", score.user.c_str());
						ImGui::TableNextColumn();
						ImGui::Text("%.2f", score.time);
						ImGui::TableNextColumn();
						char date_fmt[100];
						tm* date_tm = std::localtime(&score.updatedAt);
						std::strftime(date_fmt, 100, "%D %r", date_tm);
						ImGui::Text("%s", date_fmt);
						ImGui::TableNextColumn();
						ImGui::Text("%s", score.version.c_str());
						ImGui::TableNextColumn();
						if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), "Replay")) {
							sm->swap_state<states::edit>(m_lvl, replay(score));
						}
						ImGui::PopID();
					}
					ImGui::EndTable();
				}
			}
		}
		ImGui::EndPopup();
	}
}

void ApiLevelTile::m_update_query() {
	auto& replay_query	= context::get().replay_search_query();
	replay_query.sortBy = m_sort_opts[m_sort_selection];
	replay_query.order	= m_order_opts[m_order_selection];
	if (m_lb_handle.fetching()) return;

	if (replay_query != m_last_query) {
		m_cursor_log		= {};
		replay_query.cursor = -1;
	}

	m_lb_handle.reset(api::get().search_replays(m_lvl.id, replay_query));
	m_last_query = replay_query;
}

void ApiLevelTile::imdraw(fsm* sm) {
	// title / auth
	ImGui::PushStyleColor(ImGuiCol_Text, 0xFB8CABFF);
	ImGui::Text("%s (%s) #%d", m_lvl.title.c_str(), m_lvl.author.c_str(), m_lvl.id);
	ImGui::PopStyleColor();
	ImGui::Image(m_map_tex);

	// download btn
	std::string download_label = std::to_string(m_lvl.downloads) + "###DL";
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/download.png"), download_label.c_str())) {
		api::get().ping_download(m_lvl.id);
		sm->swap_state<states::edit>(m_lvl);
	}
	ImGui::SameLine();
	// leaderboard btn
	std::ostringstream leaderboard_label;
	if (m_lvl.record) {
		leaderboard_label << "WR: " << std::fixed << std::setprecision(2) << m_lvl.record.value() << "s";
	} else {
		leaderboard_label << "No WR yet";
	}
	leaderboard_label << "###Leaderboard";
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/trophy.png"), leaderboard_label.str().c_str())) {
		m_update_query();
		ImGui::OpenPopup("###Leaderboard");
	}
	m_gui_leaderboard_popup(sm);
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

	m_vote_handle.poll();
	m_lb_handle.poll();
	if (m_vote_handle.ready()) {
		auto status = m_vote_handle.get();
		if (status.success) {
			m_lvl.likes	   = status.level->likes;
			m_lvl.dislikes = status.level->dislikes;
			m_lvl.myVote   = status.level->myVote;
			m_vote_handle.reset();
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

void ApiLevelTile::m_next_page() {
	if (m_lb_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	m_cursor_log.push(query.cursor);
	query.cursor = m_lb_handle.get().cursor;
	m_update_query();
}

void ApiLevelTile::m_prev_page() {
	if (m_lb_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	if (m_cpage() != 0) {
		query.cursor = m_cursor_log.top();
		m_cursor_log.pop();
		m_update_query();
	}
}

bool ApiLevelTile::m_last_page() const {
	return m_lb_handle.ready() && !m_lb_handle.fetching() && m_lb_handle.get().success && m_lb_handle.get().cursor == -1;
}

int ApiLevelTile::m_cpage() const {
	return m_cursor_log.size();
}

}
