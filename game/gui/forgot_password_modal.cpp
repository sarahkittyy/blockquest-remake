#include "forgot_password_modal.hpp"

#include <SFML/Window.hpp>
#include "resource.hpp"

int forgot_password_modal::m_next_id = 0;

forgot_password_modal::forgot_password_modal() {
	m_ex_id = m_next_id++;
	m_reset();
}

void forgot_password_modal::m_reset() {
	std::memset(m_email, 0, sizeof(m_email));
	std::memset(m_code, 0, sizeof(m_code));
	std::memset(m_new_password, 0, sizeof(m_new_password));
	m_forgot_handle.reset();
	m_reset_handle.reset();
}

void forgot_password_modal::open() {
	m_reset();
	std::string modal_title = "###FGPEmail" + std::to_string(m_ex_id);
	ImGui::OpenPopup(modal_title.c_str());
}

bool forgot_password_modal::success() const {
	return m_reset_handle.ready() && m_reset_handle.get().success;
}

void forgot_password_modal::m_open_step_2() {
	std::string modal_title = "###FGPCode" + std::to_string(m_ex_id);
	m_reset_handle.reset();
	ImGui::CloseCurrentPopup();
}

void forgot_password_modal::imdraw() {
	m_forgot_handle.poll();
	m_reset_handle.poll();
	std::string modal1_title		 = "###FGPEmail" + std::to_string(m_ex_id);
	std::string modal2_title		 = "###FGPCode" + std::to_string(m_ex_id);
	ImGuiWindowFlags flags			 = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
	ImGuiInputTextFlags iflags_enter = ImGuiInputTextFlags_EnterReturnsTrue;
	// STEP 1 - fetch email and send req
	if (ImGui::BeginPopup(modal1_title.c_str(), flags)) {
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			m_reset();
			ImGui::CloseCurrentPopup();
		}
		if (m_forgot_handle.ready()) {
			auto res = m_forgot_handle.get();
			if (!res.success) {
				ImGui::TextColored(sf::Color::Red, "%s", res.error->c_str());
			} else {
				m_open_step_2();
			}
		}
		if (ImGui::InputText("Email###Emailfgp", m_email, 150, iflags_enter)) {
			if (!m_forgot_handle.fetching()) {
				m_forgot_handle.reset(auth::get().forgot_password(m_email));
			}
		}
		const char* submit_text = m_forgot_handle.fetching() ? "Submitting...###FGPSubmit1" : "Submit###FGPSubmit1";
		ImGui::BeginDisabled(m_forgot_handle.fetching());
		if (ImGui::Button(submit_text)) {
			if (!m_forgot_handle.fetching()) {
				m_forgot_handle.reset(auth::get().forgot_password(m_email));
			}
		}
		ImGui::EndDisabled();
		ImGui::EndPopup();
	} else if (m_forgot_handle.ready() && m_forgot_handle.get().success) {
		ImGui::OpenPopup(modal2_title.c_str());
	}
	// STEP 2 - fetch new password & code
	if (ImGui::BeginPopup(modal2_title.c_str(), flags)) {
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			m_reset();
			ImGui::CloseCurrentPopup();
		}
		if (m_reset_handle.ready()) {
			auto res = m_reset_handle.get();
			if (!res.success) {
				ImGui::TextColored(sf::Color::Red, "%s", res.error->c_str());
			} else {
				ImGui::CloseCurrentPopup();
			}
		} else if (m_forgot_handle.ready()) {
			auto res		 = m_forgot_handle.get();
			std::time_t diff = *res.exp - std::time(nullptr);
			int diff_minutes = diff / 60;
			ImGui::TextWrapped("Password reset code %s sent to your inbox. It expires in %d minute(s).", res.exists.value_or(false) ? "was already" : "has been", diff_minutes);
		}

		if (ImGui::ImageButton(resource::get().imtex("assets/gui/paste.png"), ImVec2(16, 16))) {
			std::string clipboard = sf::Clipboard::getString();
			std::strncpy(m_code, clipboard.c_str(), 70);
		}
		ImGui::SameLine();
		if (ImGui::InputText("Code###Codefgp", m_code, 70, iflags_enter)) {
			ImGui::SetKeyboardFocusHere(0);
		}
		if (ImGui::InputText("New Password###Pwordfgp", m_new_password, 70, ImGuiInputTextFlags_Password | iflags_enter)) {
			if (!m_reset_handle.fetching())
				m_reset_handle.reset(auth::get().reset_password(m_email, m_code, m_new_password));
		}
		ImGui::BeginDisabled(m_reset_handle.fetching());
		const char* submit_text = m_reset_handle.fetching() ? "Submitting...###SubmitFGPCodeReset" : "Submit###SubmitFGPCodeReset";
		if (ImGui::Button(submit_text)) {
			if (!m_reset_handle.fetching())
				m_reset_handle.reset(auth::get().reset_password(m_email, m_code, m_new_password));
		}
		ImGui::EndDisabled();

		ImGui::EndPopup();
	}
}
