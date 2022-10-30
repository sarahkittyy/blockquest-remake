#include "search.hpp"

#include "../debug.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "resource.hpp"
#include "util.hpp"

#include "gui/image_text_button.hpp"
#include "gui/level_card.hpp"

#include "edit.hpp"

namespace states {

search::search()
	: m_query({ .cursor = -1, .query = "", .matchTitle = true, .matchDescription = true, .sortBy = "id", .order = "desc" }),
	  m_sort_opts{ "downloads", "id", "createdAt", "updatedAtt", "title" },
	  m_sort_selection(0),
	  m_order_opts{ "asc", "desc" },
	  m_order_selection(1),
	  m_rows(3),
	  m_cols(5),
	  m_temp_rows(3),
	  m_temp_cols(5),
	  m_loading_gif(resource::get().tex("assets/gifs/loading-gif.png"), 29, { 200, 200 }, 40) {
	m_update_query();
}

search::~search() {
}

void search::update(fsm* sm, sf::Time dt) {
	m_loading_gif.update();
	// check if a pending query is ready, and update status accordingly
	if (m_query_future.valid() && util::ready(m_query_future)) {
		m_query_status = m_query_future.get();
	}
}

void search::imdraw(fsm* sm) {
	m_menu_bar.imdraw(m_menu_info);

	const float x_split = 0.26f;

	ImGuiWindowFlags flat =
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;
	sf::Vector2f wsz(resource::get().window().getSize());
	wsz.y -= 26;

	const sf::Vector2f menu_sz(wsz.x * x_split, 300);
	const sf::Vector2f results_sz(wsz.x - menu_sz.x, wsz.y);

	// menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(wsz.x - menu_sz.x, 26), ImGuiCond_Always);
	ImGui::SetNextWindowSize(menu_sz, ImGuiCond_Always);
	ImGui::Begin("Menu###Menu", nullptr, flat);
	ImGui::PopStyleVar();

	ImGui::Image(resource::get().tex("assets/logo_new.png"));
	ImGui::NewLine();
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/create.png"), "Create")) {
		sm->swap_state<states::edit>();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(true);
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/folder.png"), "Levels")) {
		// sm->swap_state<states::edit>();
	}
	ImGui::EndDisabled();

	ImGui::End();

	// query options
	ImGui::BeginDisabled(m_searching());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(wsz.x - menu_sz.x, menu_sz.y + 26), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(menu_sz.x, wsz.y - menu_sz.y), ImGuiCond_Always);
	ImGui::Begin("Search Options###Opts", nullptr, flat);
	ImGui::PopStyleVar();

	ImGui::SetNextItemWidth(160);
	if (ImGui::Combo("Sort By###SortBy", &m_sort_selection, m_sort_opts, 5)) {
		m_update_query();
	}
	ImGui::SetNextItemWidth(140);
	if (ImGui::Combo("Order By###OrderBy", &m_order_selection, m_order_opts, 2)) {
		m_update_query();
	}

	if (ImGui::InputTextWithHint("Query", "Enter keywords here...", &m_query.query,
								 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll)) {
		m_update_query();
	}
	ImGui::Checkbox("Match Title", &m_query.matchTitle);
	ImGui::Checkbox("Match Description", &m_query.matchDescription);

	ImGui::SliderInt("Rows", &m_temp_rows, 1, 4);
	ImGui::SliderInt("Cols", &m_temp_cols, 1, 5);

	if (m_query_status && !m_query_status->success) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
		ImGui::TextWrapped("%s", m_query_status->error->c_str());
		ImGui::PopStyleColor();
	}
	if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Refresh")) {
		m_update_query();
	}

	ImGui::End();
	ImGui::EndDisabled();

	// results
	ImGui::SetNextWindowPos(ImVec2(0, 26), ImGuiCond_Always);
	ImGui::SetNextWindowSize(results_sz, ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	if (m_query_status && m_query_status->success) {
		int page			 = m_cpage();
		std::string page_str = "Results (Page " + std::to_string(page) + ")###Search";
		ImGui::Begin(page_str.c_str(), nullptr, flat);
	} else {
		ImGui::Begin("Results###Search", nullptr, flat);
	}
	ImGui::PopStyleVar();

	if (m_searching()) {
		sf::Vector2i sz(100, 100);
		ImGui::SetCursorPos(ImVec2((results_sz.x - sz.x) / 2.f, (results_sz.y - sz.y) / 2.f));
		m_loading_gif.draw(sz);
	} else if (m_query_status && m_query_status->success && m_query_status->levels.size() >= 1) {
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

		// this can be unset if next/prev page is called
		ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH;
		if (m_query_status && ImGui::BeginTable("###LevelDisplay", m_cols, flags)) {
			// level table
			bool no_more_levels = false;
			for (int row = 0; row < m_rows && !no_more_levels; ++row) {
				ImGui::TableNextRow();
				for (int col = 0; col < m_cols; ++col) {
					ImGui::PushID(row * m_cols + col);
					ImGui::TableNextColumn();
					int idx = row * m_cols + col;

					api::level l = m_query_status->levels[idx];

					bool download;
					m_gui_level_tile(l).imdraw(&download);
					ImGui::PopID();
					if (download) {
						api::get().ping_download(l.id);
						sm->swap_state<states::edit>(l);
					}
					if (idx >= m_query_status->levels.size() - 1) {
						no_more_levels = true;
						break;
					}
				}
			}

			ImGui::EndTable();
		}
	}

	ImGui::End();
}

ImGui::ApiLevelTile& search::m_gui_level_tile(api::level lvl) {
	if (!m_api_level_tile.contains(lvl.id)) {
		m_api_level_tile[lvl.id] = std::make_shared<ImGui::ApiLevelTile>(lvl);
	}
	return *m_api_level_tile[lvl.id].get();
}

void search::m_next_page() {
	m_cursor_log.push(m_query.cursor);
	m_query.cursor = m_query_status->cursor;
	m_update_query();
}

void search::m_prev_page() {
	if (m_cpage() != 0) {
		m_query.cursor = m_cursor_log.top();
		m_cursor_log.pop();
		m_update_query();
	}
}

bool search::m_last_page() const {
	return m_query_status && m_query_status->success && m_query_status->cursor == -1;
}

int search::m_cpage() const {
	return m_cursor_log.size();
}

void search::m_update_query() {
	m_rows = m_temp_rows;
	m_cols = m_temp_cols;

	m_query.sortBy = m_sort_opts[m_sort_selection];
	m_query.order  = m_order_opts[m_order_selection];
	m_query.limit  = m_rows * m_cols;

	// wait for the current request to process
	if (m_query_future.valid()) return;
	debug::log() << "Search query updated\n";
	m_api_level_tile.clear();

	if (m_query != m_last_query) {
		m_cursor_log   = {};
		m_query.cursor = -1;
	}

	m_query_status.reset();
	m_query_future = api::get().search_levels(m_query);
	m_last_query   = m_query;
}

bool search::m_searching() const {
	return m_query_future.valid();
}
}
