#include "user_modal.hpp"

#include "auth.hpp"
#include "fsm.hpp"
#include "gui/image_text_button.hpp"
#include "resource.hpp"

int user_modal::m_next_id = 0;

user_modal::user_modal(int id)
	: m_id(id),
	  m_name("") {
	m_ex_id = m_next_id++;
}

user_modal::user_modal(std::string name)
	: m_id(-1),
	  m_name(name) {
	m_ex_id = m_next_id++;
}

user_modal::~user_modal() {
}

void user_modal::m_fetch() {
	if (m_stats_handle.fetching()) return;
	if (m_id != -1) {
		m_stats_handle.reset(api::get().fetch_user_stats(m_id));
	} else if (m_name.length() != 0) {
		m_stats_handle.reset(api::get().fetch_user_stats(m_name));
	}
}

void user_modal::open() {
	m_fetch();
}

void user_modal::imdraw(fsm* sm) {
	if (!m_stats_handle.ready() && !m_stats_handle.fetching()) return;
	m_stats_handle.poll();
	std::string modal_title = "Stats###UserStats";
	ImGuiWindowFlags flags	= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

	if (m_stats_handle.ready()) {
		api::user_stats_response rsp = m_stats_handle.get();
		if (rsp.success) {
			modal_title = rsp.stats->username + " #" + std::to_string(rsp.stats->id) + " " + modal_title;
			if (rsp.stats->recentLevel && !m_recent_level_tile) {
				m_recent_level_tile.reset(new level_card(*rsp.stats->recentLevel));
			}
			if (rsp.stats->recentScore && rsp.stats->recentScoreLevel && !m_recent_score_tile) {
				m_recent_score_tile.reset(new level_card(*rsp.stats->recentScoreLevel));
			}
			if (rsp.stats->pinned && !m_pinned_level_tile) {
				m_pinned_level_tile.reset(new level_card(*rsp.stats->pinned));
			}

			m_player_icon.reset(new player_icon(rsp.stats->fillColor, rsp.stats->outlineColor));
		}
	}

	modal_title += std::to_string(m_ex_id);

	bool open = m_stats_handle.ready();
	if (ImGui::Begin(modal_title.c_str(), &open, flags)) {
		if (!open && !m_stats_handle.fetching()) {
			m_stats_handle.reset();
			return ImGui::End();
		}
		if (!m_stats_handle.ready()) {
			ImGui::Text("Loading...");
		} else if (!m_stats_handle.get().success) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
			ImGui::TextWrapped("%s", m_stats_handle.get().error->c_str());
			ImGui::PopStyleColor();
		} else {
			api::user_stats stats = *m_stats_handle.get().stats;

			// player stats
			if (m_player_icon) {
				m_player_icon->imdraw(16, 16);
			} else {
				ImGui::Image(resource::get().imtex("assets/gui/play.png"), ImVec2(16, 16));
			}

			ImGui::SameLine();
			ImGui::TextColored(sf::Color::Cyan, "%s #%d", stats.username.c_str(), stats.id);

			ImGui::Image(resource::get().imtex("assets/gui/create.png"), ImVec2(16, 16));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Has created %d levels", stats.count.levels);
			}
			ImGui::SameLine();
			ImGui::Text("%d", stats.count.levels);
			ImGui::SameLine();

			ImGui::Image(resource::get().imtex("assets/gui/upload.png"), ImVec2(16, 16));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Has posted %d comments", stats.count.comments);
			}
			ImGui::SameLine();
			ImGui::Text("%d", stats.count.comments);
			ImGui::SameLine();

			ImGui::Image(resource::get().imtex("assets/gui/trophy.png"), ImVec2(16, 16));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Has submitted %d records", stats.count.scores);
			}
			ImGui::SameLine();
			ImGui::Text("%d", stats.count.scores);
			ImGui::SameLine();

			ImGui::Image(resource::get().imtex("assets/gui/heart.png"), ImVec2(16, 16));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Has liked/disliked %d levels", stats.count.votes);
			}
			ImGui::SameLine();
			ImGui::Text("%d", stats.count.votes);
			ImGui::SameLine();

			ImGui::Image(resource::get().imtex("assets/gui/crown.png"), ImVec2(16, 16));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Holds %d world records", stats.count.records);
			}
			ImGui::SameLine();
			ImGui::Text("%d", stats.count.records);

			// dynamic column
			const int column_count = int(bool(m_recent_level_tile)) + int(bool(m_recent_score_tile)) + int(bool(m_pinned_level_tile));
			if (column_count > 0 && ImGui::BeginTable("###Recencies", column_count, ImGuiTableFlags_Borders)) {
				ImGui::TableNextRow();
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color(0xC8AD7FFF)));
				if (m_pinned_level_tile) {
					ImGui::TableNextColumn();
					ImGui::Text("Pinned Level");
				}
				if (m_recent_level_tile) {
					ImGui::TableNextColumn();
					ImGui::Text("Most recent level");
				}
				if (m_recent_score_tile) {
					ImGui::TableNextColumn();
					ImGui::Text("Most recent score");
				}
				ImGui::PopStyleColor();
				ImGui::TableNextRow();
				if (m_pinned_level_tile) {
					ImGui::TableNextColumn();
					m_pinned_level_tile->imdraw(sm);
				}
				if (m_recent_level_tile) {
					ImGui::TableNextColumn();
					m_recent_level_tile->imdraw(sm);
				}
				if (m_recent_score_tile) {
					ImGui::TableNextColumn();
					auto score = *stats.recentScore;
					char date_fmt[100];
					tm* date_tm = std::localtime(&stats.recentScore->createdAt);
					std::strftime(date_fmt, 100, "%D %r", date_tm);
					ImGui::Text("%.2f set on %s", score.time, date_fmt);
					m_recent_score_tile->imdraw(sm);
				}

				ImGui::EndTable();
			}
		}
	}
	ImGui::End();
}
