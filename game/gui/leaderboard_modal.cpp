#include "leaderboard_modal.hpp"

#include "gui/image_text_button.hpp"

#include "auth.hpp"
#include "context.hpp"
#include "fsm.hpp"
#include "gui/user_modal.hpp"
#include "replay.hpp"
#include "resource.hpp"
#include "states/edit.hpp"

int leaderboard_modal::m_next_id = 0;

leaderboard_modal::leaderboard_modal(api::level& lvl)
	: m_lvl(lvl),
	  m_sort_opts{ "time", "author", "createdAt", "updatedAt" },
	  m_order_opts{ "asc", "desc" } {
	auto replay_query = context::get().replay_search_query();
	auto sort_it	  = std::find_if(std::begin(m_sort_opts), std::end(m_sort_opts),
									 [this, replay_query](const char* str) { return std::string(str) == replay_query.sortBy; });
	m_sort_selection  = sort_it != std::end(m_sort_opts) ? std::distance(std::begin(m_sort_opts), sort_it) : 0;
	auto order_it	  = std::find_if(std::begin(m_order_opts), std::end(m_order_opts),
									 [this, replay_query](const char* str) { return std::string(str) == replay_query.order; });
	m_order_selection = sort_it != std::end(m_order_opts) ? std::distance(std::begin(m_order_opts), order_it) : 0;

	m_ex_id = m_next_id++;
}

leaderboard_modal::~leaderboard_modal() {
}

void leaderboard_modal::open() {
	m_update_query();
	std::string modal_title = "###Leaderboard" + std::to_string(m_ex_id);
	ImGui::OpenPopup(modal_title.c_str());
}

int leaderboard_modal::m_cpage() const {
	return m_cursor_log.size();
}

bool leaderboard_modal::m_last_page() const {
	return m_api_handle.ready() && !m_api_handle.fetching() && m_api_handle.get().success && m_api_handle.get().cursor == -1;
}

void leaderboard_modal::m_next_page() {
	if (m_api_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	m_cursor_log.push(query.cursor);
	query.cursor = m_api_handle.get().cursor;
	m_update_query();
}

void leaderboard_modal::m_prev_page() {
	if (m_api_handle.fetching()) return;
	auto& query = context::get().replay_search_query();
	if (m_cpage() != 0) {
		query.cursor = m_cursor_log.top();
		m_cursor_log.pop();
		m_update_query();
	}
}

void leaderboard_modal::m_update_query() {
	auto& replay_query	= context::get().replay_search_query();
	replay_query.sortBy = m_sort_opts[m_sort_selection];
	replay_query.order	= m_order_opts[m_order_selection];
	if (m_api_handle.fetching()) return;

	if (replay_query != m_last_query) {
		m_cursor_log		= {};
		replay_query.cursor = -1;
	}

	m_api_handle.reset(api::get().search_replays(m_lvl.id, replay_query));
	m_last_query = replay_query;
}

void leaderboard_modal::imdraw(fsm* sm) {
	m_api_handle.poll();
	std::string modal_title = "###Leaderboard" + std::to_string(m_ex_id);
	ImGuiWindowFlags flags	= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopup(modal_title.c_str(), flags)) {
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}
		// query opts
		ImGui::BeginDisabled(m_api_handle.fetching());
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
		if (m_api_handle.ready()) {
			api::replay_search_response res = m_api_handle.get();
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
							if (score == wr) {
								ImGui::Image(resource::get().imtex("assets/gui/crown.png"), sf::Vector2f(16, 16));
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("World Record");
								}
								ImGui::SameLine();
							}
							if (m_lvl.verificationId.has_value() && *m_lvl.verificationId == score.id) {
								ImGui::Image(resource::get().imtex("assets/gui/home.png"), sf::Vector2f(16, 16));
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Original verification");
								}
								ImGui::SameLine();
							}
							ImGui::BeginGroup();
							ImGui::Text("%s%s", score.user.c_str(), auth::get().authed() && auth::get().username() == score.user ? " (You)" : "");
							if (score.user == m_lvl.author) {
								ImGui::SameLine();
								ImGui::Image(resource::get().imtex("assets/gui/create.png"), sf::Vector2f(16, 16));
								if (ImGui::IsItemHovered()) {
									ImGui::SetTooltip("Level Creator");
								}
							}
							ImGui::EndGroup();
							if (ImGui::IsItemHovered()) {
								ImGui::SetTooltip("View player profile");
								ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
								if (ImGui::IsMouseClicked(0)) {
									m_user_modal.reset(new user_modal(score.user));
									m_user_modal->open();
								}
							}
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
		if (m_user_modal) m_user_modal->imdraw(sm);
		ImGui::EndPopup();
	}
}
