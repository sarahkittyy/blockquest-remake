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
	  m_lb_sort_opts{ "time", "author", "createdAt", "updatedAt" },
	  m_lb_order_opts{ "asc", "desc" },
	  m_comment_sort_opts{ "createdAt" },
	  m_comment_order_opts{ "asc", "desc" } {
	auto replay_query	 = context::get().replay_search_query();
	auto lb_sort_it		 = std::find_if(std::begin(m_lb_sort_opts), std::end(m_lb_sort_opts),
										[this, replay_query](const char* str) { return std::string(str) == replay_query.sortBy; });
	m_lb_sort_selection	 = lb_sort_it != std::end(m_lb_sort_opts) ? std::distance(std::begin(m_lb_sort_opts), lb_sort_it) : 0;
	auto lb_order_it	 = std::find_if(std::begin(m_lb_order_opts), std::end(m_lb_order_opts),
										[this, replay_query](const char* str) { return std::string(str) == replay_query.order; });
	m_lb_order_selection = lb_sort_it != std::end(m_lb_order_opts) ? std::distance(std::begin(m_lb_order_opts), lb_order_it) : 0;

	auto comment_query		  = context::get().comment_search_query();
	auto comment_sort_it	  = std::find_if(std::begin(m_comment_sort_opts), std::end(m_comment_sort_opts),
											 [this, comment_query](const char* str) { return std::string(str) == comment_query.sortBy; });
	m_comment_sort_selection  = comment_sort_it != std::end(m_comment_sort_opts) ? std::distance(std::begin(m_comment_sort_opts), comment_sort_it) : 0;
	auto comment_order_it	  = std::find_if(std::begin(m_comment_order_opts), std::end(m_comment_order_opts),
											 [this, comment_query](const char* str) { return std::string(str) == comment_query.order; });
	m_comment_order_selection = comment_sort_it != std::end(m_comment_order_opts) ? std::distance(std::begin(m_comment_order_opts), comment_order_it) : 1;

	m_map_tex.create(256, 256);
	m_tmap.load(m_lvl.code);
	m_map_tex.setView(sf::View(sf::FloatRect(0, 0, 512, 512)));
	m_map_tex.setSmooth(true);
	m_map_tex.clear(m_bg);
	m_map_tex.draw(m_tmap);
	m_map_tex.display();

	std::memset(m_comment_buffer, 0, 251);
}

void ApiLevelTile::m_gui_comments_popup(fsm* sm) {
	bool dummy				= true;
	std::string modal_title = m_lvl.title + " comments###Comments";
	ImGuiWindowFlags flags	= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopupModal(modal_title.c_str(), &dummy, flags)) {
		// query opts
		ImGui::BeginDisabled(m_lb_handle.fetching());
		auto& comment_query = context::get().comment_search_query();
		ImGui::SetNextItemWidth(160);
		if (ImGui::Combo("Sort By###SortBy", &m_comment_sort_selection, m_comment_sort_opts, sizeof(m_comment_sort_opts) / sizeof(m_comment_sort_opts[0]))) {
			m_update_comment_query();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140);
		if (ImGui::Combo("Order By###OrderBy", &m_comment_order_selection, m_comment_order_opts, sizeof(m_comment_order_opts) / sizeof(m_comment_order_opts[0]))) {
			m_update_comment_query();
		}
		ImGui::SameLine();
		ImGui::SliderInt("Limit###Limit", &comment_query.limit, 1, 20, "%d", ImGuiSliderFlags_NoInput);
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Refresh")) {
			m_update_comment_query();
		}
		ImGui::EndDisabled();
		if (m_comment_handle.ready()) {
			api::comment_search_response res = m_comment_handle.get();
			// pagination
			int page = m_comment_cpage();
			ImGui::BeginDisabled(page == 0);
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Back")) {
				m_comment_prev_page();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(m_comment_last_page());
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/forward.png"), "Next")) {
				m_comment_next_page();
			}
			ImGui::EndDisabled();
			// comment table
			if (!res.success) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", res.error->c_str());
				ImGui::PopStyleColor();
			} else {
				if (ImGui::BeginTable("###Comments", 2, ImGuiTableFlags_Borders)) {
					ImGui::TableNextRow();
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color(0xC8AD7FFF)));
					ImGui::TableNextColumn();
					ImGui::Text("Date");
					ImGui::TableNextColumn();
					ImGui::Text("Comments");
					ImGui::PopStyleColor();
					for (int i = 0; i < res.comments.size(); ++i) {
						ImGui::PushID(i);
						ImGui::TableNextRow();
						auto& comment = res.comments[i];
						ImGui::TableNextColumn();
						char date_fmt[100];
						tm* date_tm = std::localtime(&comment.createdAt);
						std::strftime(date_fmt, 100, "%D %r", date_tm);
						ImGui::Text("%s", date_fmt);
						ImGui::TableNextColumn();
						ImGui::Text("[%s]#%d %s",
									comment.user.name.c_str(), comment.id, comment.text.c_str());
						ImGui::PopID();
					}
					ImGui::EndTable();
				}
			}
		}
		ImGui::Separator();
		// comment box
		ImVec2 avail_size	  = ImGui::GetWindowSize();
		bool can_post_comment = !m_comment_post_handle.fetching() && auth::get().authed();
		ImGui::BeginDisabled(!can_post_comment);
		bool comment_posted = ImGui::InputTextMultiline(
			"###CommentPost",
			m_comment_buffer,
			sizeof(m_comment_buffer),
			ImVec2(avail_size.x * 0.8f, 64),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CtrlEnterForNewLine);
		comment_posted |= ImGui::ImageButtonWithText(
			resource::get().imtex("assets/gui/forward.png"),
			"Post Comment###PostCommentButton");
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			if (!auth::get().authed()) {
				ImGui::SetTooltip("Log in to post comments!");
			} else if (m_comment_post_handle.fetching()) {
				ImGui::SetTooltip("Posting...");
			}
		}
		if (comment_posted && can_post_comment) {
			m_comment_post_handle.reset(api::get().post_comment(m_lvl.id, std::string(m_comment_buffer)));
		}
		m_comment_post_handle.poll();
		if (m_comment_post_handle.ready() && !m_comment_post_handle.fetching()) {
			api::comment_response res = m_comment_post_handle.get();
			if (!res.success) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", res.error->c_str());
				ImGui::PopStyleColor();
			} else {
				std::memset(m_comment_buffer, 0, 251);
				m_update_comment_query();
				m_comment_post_handle.reset();
			}
		}
		ImGui::EndPopup();
	}
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
		if (ImGui::Combo("Sort By###SortBy", &m_lb_sort_selection, m_lb_sort_opts, sizeof(m_lb_sort_opts) / sizeof(m_lb_sort_opts[0]))) {
			m_update_lb_query();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140);
		if (ImGui::Combo("Order By###OrderBy", &m_lb_order_selection, m_lb_order_opts, sizeof(m_lb_order_opts) / sizeof(m_lb_order_opts[0]))) {
			m_update_lb_query();
		}
		ImGui::SameLine();
		ImGui::SliderInt("Limit###Limit", &replay_query.limit, 1, 20, "%d", ImGuiSliderFlags_NoInput);
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Refresh")) {
			m_update_lb_query();
		}
		ImGui::EndDisabled();
		ImGui::Separator();
		if (m_lb_handle.ready()) {
			api::replay_search_response res = m_lb_handle.get();
			// pagination buttons
			int page = m_lb_cpage();
			ImGui::BeginDisabled(page == 0);
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/back.png"), "Back")) {
				m_lb_prev_page();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(m_lb_last_page());
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/forward.png"), "Next")) {
				m_lb_next_page();
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
					if (res.scores.size()) {
						api::replay wr = res.scores[0];
						for (auto& score : res.scores) {
							if (score.time < wr.time) {
								wr = score;
							}
						}
						for (int i = 0; i < res.scores.size(); ++i) {
							ImGui::PushID(i);
							ImGui::TableNextRow();
							auto& score = res.scores[i];
							ImGui::TableNextColumn();
							if (score.user == m_lvl.author) {
								ImGui::Image(resource::get().imtex("assets/gui/create.png"), sf::Vector2f(16, 16));
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Level Creator");
								}
								ImGui::SameLine();
							}
							if (score == wr) {
								ImGui::Image(resource::get().imtex("assets/gui/crown.png"), sf::Vector2f(16, 16));
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("World Record");
								}
								ImGui::SameLine();
							}
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
					}
					ImGui::EndTable();
				}
			}
		}
		ImGui::EndPopup();
	}
}

void ApiLevelTile::m_update_lb_query() {
	auto& replay_query	= context::get().replay_search_query();
	replay_query.sortBy = m_lb_sort_opts[m_lb_sort_selection];
	replay_query.order	= m_lb_order_opts[m_lb_order_selection];
	if (m_lb_handle.fetching()) return;

	if (replay_query != m_last_replay_query) {
		m_lb_cursor_log		= {};
		replay_query.cursor = -1;
	}

	m_lb_handle.reset(api::get().search_replays(m_lvl.id, replay_query));
	m_last_replay_query = replay_query;
}

void ApiLevelTile::m_update_comment_query() {
	auto& comment_query	 = context::get().comment_search_query();
	comment_query.sortBy = m_comment_sort_opts[m_comment_sort_selection];
	comment_query.order	 = m_comment_order_opts[m_comment_order_selection];
	if (m_comment_handle.fetching()) return;

	if (comment_query != m_last_comment_query) {
		m_comment_cursor_log = {};
		comment_query.cursor = -1;
	}

	m_comment_handle.reset(api::get().get_comments(m_lvl.id, comment_query));
	m_last_comment_query = comment_query;
}

void ApiLevelTile::imdraw(fsm* sm) {
	// title / auth
	ImGui::PushStyleColor(ImGuiCol_Text, 0xFB8CABFF);
	ImGui::Text("%s (%s) #%d", m_lvl.title.c_str(), m_lvl.author.c_str(), m_lvl.id);
	ImGui::PopStyleColor();
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
	leaderboard_label << "###Leaderboard";
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/trophy.png"), leaderboard_label.str().c_str())) {
		m_update_lb_query();
		ImGui::OpenPopup("###Leaderboard");
	}

	// likes/dislikes/downloads/records
	ImVec2 x16(16, 16);
	ImVec2 uv0(0, 0), uv1(1, 1);
	int fp				  = 4;
	ImVec4 bg			  = ImVec4(1, 0, 0, 0);
	std::string likes	  = std::to_string(m_lvl.likes) + "###LIKES";
	std::string dislikes  = std::to_string(m_lvl.dislikes) + "###DISLIKES";
	std::string downloads = std::to_string(m_lvl.downloads) + "###DOWNLOADS";
	std::string comments  = std::to_string(m_lvl.comments) + "###COMMENTS";
	std::string records	  = std::to_string(m_lvl.records) + "###RECORDS";
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
		m_update_lb_query();
		ImGui::OpenPopup("###Leaderboard");
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Scores posted");
	}

	ImGui::SameLine();

	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/comment.png"), comments.c_str(), x16, uv0, uv1, fp, bg)) {
		m_update_comment_query();
		ImGui::OpenPopup("###Comments");
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Comments");
	}

	m_vote_handle.poll();
	m_lb_handle.poll();
	m_comment_handle.poll();
	if (m_vote_handle.ready()) {
		auto status = m_vote_handle.get();
		if (status.success) {
			m_lvl.likes	   = status.level->likes;
			m_lvl.dislikes = status.level->dislikes;
			m_lvl.myVote   = status.level->myVote;
			m_vote_handle.reset();
		}
	}

	m_gui_leaderboard_popup(sm);
	m_gui_comments_popup(sm);

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

void ApiLevelTile::m_comment_next_page() {
	if (m_comment_handle.fetching()) return;
	auto& query = context::get().comment_search_query();
	m_comment_cursor_log.push(query.cursor);
	query.cursor = m_comment_handle.get().cursor;
	m_update_comment_query();
}

void ApiLevelTile::m_comment_prev_page() {
	if (m_comment_handle.fetching()) return;
	auto& query = context::get().comment_search_query();
	if (m_comment_cpage() != 0) {
		query.cursor = m_comment_cursor_log.top();
		m_comment_cursor_log.pop();
		m_update_comment_query();
	}
}

bool ApiLevelTile::m_comment_last_page() const {
	return m_comment_handle.ready() && !m_comment_handle.fetching() && m_comment_handle.get().success && m_comment_handle.get().cursor == -1;
}

int ApiLevelTile::m_comment_cpage() const {
	return m_comment_cursor_log.size();
}

void ApiLevelTile::m_lb_next_page() {
	if (m_lb_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	m_lb_cursor_log.push(query.cursor);
	query.cursor = m_lb_handle.get().cursor;
	m_update_lb_query();
}

void ApiLevelTile::m_lb_prev_page() {
	if (m_lb_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	if (m_lb_cpage() != 0) {
		query.cursor = m_lb_cursor_log.top();
		m_lb_cursor_log.pop();
		m_update_lb_query();
	}
}

bool ApiLevelTile::m_lb_last_page() const {
	return m_lb_handle.ready() && !m_lb_handle.fetching() && m_lb_handle.get().success && m_lb_handle.get().cursor == -1;
}

int ApiLevelTile::m_lb_cpage() const {
	return m_lb_cursor_log.size();
}

}
