#include "app.hpp"

#include "imgui-SFML.h"

#include <fstream>
#include <iostream>

#include "auth.hpp"
#include "context.hpp"
#include "debug.hpp"

#include "gui/image_text_button.hpp"
#include "imgui-SFML.h"
#include "imgui.h"

#include "states/debug.hpp"
#include "states/edit.hpp"
#include "states/search.hpp"

#include "api.hpp"
#include "util.hpp"

app::app(int argc, char** argv) {
	if (!ImGui::SFML::Init(resource::get().window()))
		throw "Could not initialize ImGui";

	configure_imgui_style();

	if (const char* name = std::getenv("BQR_USERNAME")) {
		if (const char* pass = std::getenv("BQR_PASSWORD")) {
			auth::get().login(std::string(name), std::string(pass));
		}
	}

	m_fsm.swap_state<states::search>();

#ifndef NDEBUG
	if (argc > 1) {
		std::string mode = argv[1];
		if (mode == "search") {
			m_fsm.swap_state<states::search>();
		} else if (mode == "debug") {
			m_fsm.swap_state<states::debug>();
		} else if (mode == "edit") {
			m_fsm.swap_state<states::edit>();
		}
	}
#endif
}

int app::run() {
	sf::Clock delta_clock;	 // clock for measuring time deltas between frames
	sf::Event evt;

	sf::RenderWindow& win = resource::get().window();

	std::future<api::update_response> version_future = api::get().is_up_to_date();
	std::optional<api::update_response> version_resp;
	bool version_ack = false;

	// app loop
	while (win.isOpen()) {
		while (win.pollEvent(evt)) {
			ImGui::SFML::ProcessEvent(evt);
			m_fsm.process_event(evt);
			context::get().process_event(evt);
			switch (evt.type) {
			default:
				break;
			case sf::Event::Closed:
				win.close();
				break;
			case sf::Event::Resized:
				sf::FloatRect visibleArea(0, 0, evt.size.width, evt.size.height);
				win.setView(sf::View(visibleArea));
				break;
			}
		}
		// updates
		sf::Time dt = delta_clock.restart();
		ImGui::SFML::Update(win, dt);
		debug::get().flush();
		m_fsm.update(dt);

		// imgui rendering
		m_fsm.imdraw();
		debug::get().imdraw(dt);

		// versioning / update stuff
		if (version_future.valid() && util::ready(version_future)) {
			version_resp = version_future.get();
		}
		if (!version_ack && version_resp.has_value()) {
			if (!version_resp->success) {
				ImGui::OpenPopup("Warning###UpdateWarning");
			} else if (!*version_resp->up_to_date) {
				ImGui::OpenPopup("A new update is available!###UpdateError");
			}
		}
		bool dummy			   = true;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Warning###UpdateWarning", &dummy, flags)) {
			version_ack = true;
			ImGui::TextWrapped("Could not run the updater - %s", version_resp->error->c_str());
			ImGui::TextWrapped("Some things may not work correctly if you are on a version too old!");
			ImGui::Text("Current version: %s\n", api::get().version());
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/yes.png"), "Ok"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("A new update is available!###UpdateError", &dummy, flags)) {
			version_ack = true;
			ImGui::TextWrapped("An update from version %s to %s is available.", api::get().version(), version_resp->latest_version->c_str());
			ImGui::TextWrapped("Run the game through launcher.exe to update.");
			if (ImGui::ImageButtonWithText(resource::get().imtex("assets/gui/yes.png"), "Ok"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		ImGui::EndFrame();

		win.clear(m_fsm.bg());

		// draw
		win.draw(m_fsm);
		win.draw(debug::get());

		ImGui::SFML::Render(win);
		win.display();
	}

	ImGui::SFML::Shutdown(win);

	return 0;
}

void app::configure_imgui_style() {
	ImGuiStyle& style	  = ImGui::GetStyle();
	style.FrameRounding	  = 6;
	style.FrameBorderSize = 1;
	style.WindowRounding  = 12;
	style.TabBorderSize	  = 1;
	style.FramePadding	  = ImVec2(18, 7);

	// font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("assets/verdana.ttf", 14);
	io.Fonts->Build();
	if (!ImGui::SFML::UpdateFontTexture())
		throw "Could not update ImGui::SFML font texture";

	// color
	ImVec4* colors						   = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]				   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]		   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]			   = ImVec4(0.00f, 0.00f, 0.00f, 0.88f);
	colors[ImGuiCol_ChildBg]			   = ImVec4(0.16f, 0.10f, 0.00f, 1.00f);
	colors[ImGuiCol_PopupBg]			   = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]				   = ImVec4(0.75f, 0.52f, 0.16f, 1.00f);
	colors[ImGuiCol_BorderShadow]		   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBg]			   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_FrameBgActive]		   = ImVec4(0.64f, 0.37f, 0.01f, 1.00f);
	colors[ImGuiCol_TitleBg]			   = ImVec4(0.13f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive]		   = ImVec4(0.34f, 0.20f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]	   = ImVec4(0.16f, 0.09f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg]			   = ImVec4(0.39f, 0.22f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]		   = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]		   = ImVec4(0.44f, 0.25f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_CheckMark]			   = ImVec4(0.75f, 0.52f, 0.16f, 1.00f);
	colors[ImGuiCol_SliderGrab]			   = ImVec4(0.81f, 0.46f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]	   = ImVec4(1.00f, 0.67f, 0.24f, 1.00f);
	colors[ImGuiCol_Button]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonActive]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_Header]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_HeaderHovered]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_HeaderActive]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_Separator]			   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]	   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_SeparatorActive]	   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_ResizeGrip]			   = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]	   = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]	   = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab]				   = ImVec4(0.31f, 0.18f, 0.00f, 1.00f);
	colors[ImGuiCol_TabHovered]			   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TabActive]			   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocused]		   = ImVec4(0.14f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]	   = ImVec4(0.23f, 0.13f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines]			   = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]	   = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]		   = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]	   = ImVec4(0.39f, 0.22f, 0.00f, 1.00f);
	colors[ImGuiCol_TableBorderLight]	   = ImVec4(0.23f, 0.13f, 0.00f, 1.00f);
	colors[ImGuiCol_TableRowBg]			   = ImVec4(0.13f, 0.08f, 0.00f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt]		   = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg]		   = ImVec4(0.76f, 0.43f, 0.00f, 1.00f);
	colors[ImGuiCol_DragDropTarget]		   = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]		   = ImVec4(0.59f, 0.34f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]	   = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]	   = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

int main(int argc, char** argv) {
	app app(argc, argv);
	return app.run();
}
