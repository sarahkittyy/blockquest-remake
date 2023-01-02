#include "comment_modal.hpp"

#include "auth.hpp"
#include "context.hpp"
#include "fsm.hpp"
#include "gui/image_text_button.hpp"
#include "resource.hpp"

comment_modal::comment_modal(api::level& lvl)
	: m_lvl(lvl),
	  m_sort_opts{ "createdAt" },
	  m_order_opts{ "asc", "desc" } {
	auto comment_query = context::get().comment_search_query();
	auto sort_it	   = std::find_if(std::begin(m_sort_opts), std::end(m_sort_opts),
									  [this, comment_query](const char* str) { return std::string(str) == comment_query.sortBy; });
	m_sort_selection   = sort_it != std::end(m_sort_opts) ? std::distance(std::begin(m_sort_opts), sort_it) : 0;
	auto order_it	   = std::find_if(std::begin(m_order_opts), std::end(m_order_opts),
									  [this, comment_query](const char* str) { return std::string(str) == comment_query.order; });
	m_order_selection  = sort_it != std::end(m_order_opts) ? std::distance(std::begin(m_order_opts), order_it) : 1;

	std::memset(m_comment_buffer, 0, 251);
}

comment_modal::~comment_modal() {
}

void comment_modal::open() {
	m_update_query();
	ImGui::OpenPopup("###Comments");
}

void comment_modal::m_update_query() {
	auto& comment_query	 = context::get().comment_search_query();
	comment_query.sortBy = m_sort_opts[m_sort_selection];
	comment_query.order	 = m_order_opts[m_order_selection];
	if (m_fetch_handle.fetching()) return;

	if (comment_query != m_last_query) {
		m_cursor_log		 = {};
		comment_query.cursor = -1;
	}

	m_fetch_handle.reset(api::get().get_comments(m_lvl.id, comment_query));
	m_last_query = comment_query;
}

void comment_modal::m_next_page() {
	if (m_fetch_handle.fetching()) return;
	auto& query = context::get().comment_search_query();
	m_cursor_log.push(query.cursor);
	query.cursor = m_fetch_handle.get().cursor;
	m_update_query();
}

void comment_modal::m_prev_page() {
	if (m_fetch_handle.fetching()) return;
	auto& query = context::get().comment_search_query();
	if (m_cpage() != 0) {
		query.cursor = m_cursor_log.top();
		m_cursor_log.pop();
		m_update_query();
	}
}

bool comment_modal::m_last_page() const {
	return m_fetch_handle.ready() && !m_fetch_handle.fetching() && m_fetch_handle.get().success && m_fetch_handle.get().cursor == -1;
}

int comment_modal::m_cpage() const {
	return m_cursor_log.size();
}

void comment_modal::imdraw(fsm* sm) {
	m_fetch_handle.poll();
	bool dummy				= true;
	std::string modal_title = m_lvl.title + " comments###Comments";
	ImGuiWindowFlags flags	= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::BeginPopupModal(modal_title.c_str(), &dummy, flags)) {
		// query opts
		ImGui::BeginDisabled(m_fetch_handle.fetching());
		auto& comment_query = context::get().comment_search_query();
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
		ImGui::SliderInt("Limit###Limit", &comment_query.limit, 1, 20, "%d", ImGuiSliderFlags_NoInput);
		ImGui::SameLine();
		if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/search.png"), "Refresh")) {
			m_update_query();
		}
		ImGui::EndDisabled();
		if (m_fetch_handle.ready()) {
			api::comment_search_response res = m_fetch_handle.get();
			// pagination
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
		bool can_post_comment = !m_post_handle.fetching() && auth::get().authed();
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
			} else if (m_post_handle.fetching()) {
				ImGui::SetTooltip("Posting...");
			}
		}
		if (comment_posted && can_post_comment) {
			m_post_handle.reset(api::get().post_comment(m_lvl.id, std::string(m_comment_buffer)));
		}
		m_post_handle.poll();
		if (m_post_handle.ready() && !m_post_handle.fetching()) {
			api::comment_response res = m_post_handle.get();
			if (!res.success) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(sf::Color::Red));
				ImGui::TextWrapped("%s", res.error->c_str());
				ImGui::PopStyleColor();
			} else {
				std::memset(m_comment_buffer, 0, 251);
				m_update_query();
				m_post_handle.reset();
			}
		}
		ImGui::EndPopup();
	}
}
